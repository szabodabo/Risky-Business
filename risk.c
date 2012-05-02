#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#include "risk_input.h"

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
	int data_offset; //Offset for our territories

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

	//Once we get here, allreduce has so nicely populated total_tt for everyone
	
	int* troopCounts_temp = calloc( total_tt, sizeof(int) );
	int* teamIDs_temp = calloc( total_tt, sizeof(int) );
	
	data_offset = read_from_file( total_tt, adjMatrix, troopCounts_temp, teamIDs_temp, myRank, commSize );

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
			printf("%9d | %10d\n", i+data_offset, troopCounts[i]);
		}
	}

	printf("MPI Rank %d initalized\n", myRank);
	sleep(myRank + 1);
	//DEBUG
	int j;
	for(i = 0; i < tt_per_rank; i++) {
		printf("TT %d : ", i+data_offset);
		for(j = 0; j < total_tt; j++) {
			printf("%d ", adjMatrix[i][j]);
		}
		printf("\n");
	}	       

	MPI_Finalize();
	return EXIT_SUCCESS;
}
