#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#define ASSIGN_ATTACK(E, NUM) (E = NUM)
#define ASSIGN_DEFENSE(E, NUM) (E = (-1) * NUM)

#include "risk_input.h"

//GIANT IDEA:
//Rule change --> it is totally legal to not leave any troops in your home node. If all troops from a node
//get killed while away and nobody is left home, that's fine. That node just has 0 troops to allocate during
//every phase and it will get conquered pretty damn fast (battle resolution should report no troops lost if a team
//fights an army with 0 troops)

//Apply the strategy for team <teamID> on country <territory> by looking at the troop counts and controlling teams for 
//each node on the map. Use the adjacency matrix to figure out our neighbors.  Place results in edgeActivity
void apply_strategy(int territory, int tt_total, int *troopCounts, int *teamIDs, int *adjList, int *outboundEdge)
{
	//simple strategy to test and debug with - split troops evenly on all edges, all attacking
	//if there is a remainder of troops, don't do anything with them
	int myTroops = troopCounts[territory];
	int neighborCount = 0;
	int i;
	for( i = 0; i < tt_total; i++ ) {
		//don't need to attack my own team
		neighborCount += ( adjList[i] == 1 ) && ( teamIDs[i] != teamIDs[territory] );
	}
	
	//int remainder = myTroops % neighborCount;
	//int troopsPerRival = (myTroops - remainder) / neighborCount;
	//Can't we use integer division here?
	int troopsPerRival = myTroops / neighborCount;

	//Place troops on each edge, all attacking.
	for( i = 0; i < tt_total; i++ ) {
		if( adjList[i] == 1 && teamIDs[i] != teamIDs[territory] ) {
			ASSIGN_ATTACK(outboundEdge[i], troopsPerRival);
		}
	}
}

void print_graph( int myRank, int **adj, int *scores, int **edges, int total_tt ) {
	int i;
	for (int i = 0; i < )
}

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i;

	int total_tt = 0; //Total number of territories being warred
	
	//Everyone has each of these arrays in FULL
	int *troopCounts; //Number of troops each territory has
	int *teamIDs; //What team is each territory under control of? (team ID = starting node # for now)

	//Everyone has their own slice of these arrays
	int **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge data (passed around)

	int tt_per_rank; //Number of territories per MPI Rank
	int tt_offset; //Which territory do we start with? (which is the first territory in our set)

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	if (myRank == 0) {
		printf("Commsize is %d\n", commSize);
	}

	//Rank 0 reads header information and sends to everybody else
	if (myRank == 0) {
		read_header_info( &total_tt );
	}

	//Use all-reduce to ensure all processes are aware of the total number of territories
	int temp_tt;
	MPI_Allreduce( &total_tt, &temp_tt, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	total_tt = temp_tt;
	
	//Now use the known total territory count to init other variables
	tt_per_rank = total_tt / commSize;
	
	troopCounts = calloc( total_tt, sizeof(int) );
	teamIDs = calloc ( total_tt, sizeof(int) );

	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	adjMatrix = calloc( tt_per_rank, sizeof(int *) );

	for (i = 0; i < tt_per_rank; i++) {
		edgeActivity[i] = calloc( total_tt, sizeof(int) );
		adjMatrix[i] = calloc( total_tt, sizeof(int) );
	}

	//Temp variables to enable all-reduce (might use these later when communicating further results)	
	int* troopCounts_temp = calloc( total_tt, sizeof(int) );
	int* teamIDs_temp = calloc( total_tt, sizeof(int) );
	
	tt_offset = read_from_file( total_tt, adjMatrix, troopCounts_temp, teamIDs_temp, myRank, commSize );

	//Use all-reduce to ensure all processes are aware of initial team IDs and troop counts
	MPI_Allreduce( troopCounts_temp, troopCounts, total_tt, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, total_tt, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

	free(troopCounts_temp);
	free(teamIDs_temp);
	
	if (myRank == 0) {
		printf("total_tt is %d\n", total_tt);
		printf("==============================================\n");
		printf("Current Total Troop Counts:\n");
		printf("==============================================\n");
		printf("TERRITORY | NUM_TROOPS\n");
		for (i = 0; i < total_tt; i++) {
			printf("%9d | %10d\n", i+tt_offset, troopCounts[i]);
		}
	}

	printf("MPI Rank %d initalized\n", myRank);
	sleep(myRank + 1);
	//DEBUG
	int j;
	for(i = 0; i < tt_per_rank; i++) {
		printf("TT %d : ", i+tt_offset);
		for(j = 0; j < total_tt; j++) {
			printf("%d ", adjMatrix[i][j]);
		}
		printf("\n");
	}

	//MAIN LOOP (set up for one iteration for now)
	
	for( i = 0; i < tt_per_rank; i++ ) {
		//Apply a battle strategy for each node this Rank is responsible for
		apply_strategy( tt_offset+i, total_tt, troopCounts, teamIDs, adjMatrix[i], edgeActivity[i] );
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}
