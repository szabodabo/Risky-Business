#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mpi.h"

#define ASSIGN_ATTACK(E, NUM) (E = NUM)
#define ASSIGN_DEFENSE(E, NUM) (E = (-1) * NUM)
#define I_HAVE(TERR) (TERR > tt_offset && TERR < (tt_offset + tt_per_rank))
#define NEXT_RANK_FROM(R) (R+1 == commSize ? 0 : R+1)
#define PREV_RANK_FROM(R) ((R == 0) ? commSize-1 : R-1)
#define RECV buffer_switch
#define SEND !buffer_switch
#define HEADS 1 //..okayface.jpg
#define TAILS 2
#define COIN_FLIP 2
#define ACC(A, R, C) A[tt_total * R + C]
#define DEBUG() //printf("[%d] DEBUG %d\n", myRank, debug_num++);

#define HIT_DICE 6
#define HIT_ROLL_NORMAL 3
#define HIT_ROLL_DEF 4
#define DEFEND 0
#define ATTACK 1

#include "risk_input.h"

int tt_per_rank = 0; //Number of territories per MPI Rank
int tt_offset = 0; //Which territory do we start with? (which is the first territory in our set)
int tt_total = 0; //Total number of territories being warred

int debug_num = 0;

typedef enum answer_t {
	YES, NO
} ANSWER;

typedef struct edge_result_t {
	int myTerr; //ID of territory A
	int otherTerr; //ID of territory B
	int myTroops; //Troops owned by A
	int otherTroops; //Troops owned by B
	int myAction; //Attack or defend 
	int otherAction; //Attack or defend
} EDGE_RESULT;

int diceRoll( int sides ) {
	int randInt = rand() % sides;
	return randInt + 1;
}

//If either argument is negative, that team is defending
EDGE_RESULT do_battle(int terrA, int terrB, int troopsA, int troopsB)
{
	EDGE_RESULT result;
	result.myTerr = terrA;
	result.otherTerr = terrB;

	int actionA = troopsA < 0 ? DEFEND : ATTACK;
	int actionB = troopsB < 0 ? DEFEND : ATTACK;

	troopsA = abs(troopsA);
	troopsB = abs(troopsB);

	int r = troopsA - troopsB;
	if(r < 0)
	{
		troopsA = 0;
		troopsB = -r;
	}
	else
	{
		troopsA = r;
		troopsB = 0;
	}

	result.myTroops = troopsA;
	result.otherTroops = troopsB;
	result.myAction = actionA;
	result.otherAction = actionB;

	//if(actionA == DEFEND && actionB == DEFEND)
		return result;
/*
	//if the other team is defending, your hit chance decreases
	int a_hit = actionB == DEFEND ? HIT_ROLL_DEF : HIT_ROLL_NORMAL;
	int b_hit = actionA == DEFEND ? HIT_ROLL_DEF : HIT_ROLL_NORMAL;

	int i;
	int a_score = 0, b_score = 0;

	while(troopsA > 0 && troopsB > 0)
	{
		for(i = 0; i < troopsA; i++)
			a_score += diceRoll(HIT_DICE) > a_hit;
		for(i = 0; i < troopsB; i++)
			b_score += diceRoll(HIT_DICE) > b_hit;

		troopsA -= a_score;
		troopsB -= b_score;
	}

	troopsA = troopsA < 0 ? 0 : troopsA;
	troopsB = troopsB < 0 ? 0 : troopsB;

	result.myTroops = troopsA;
	result.otherTroops = troopsB;

	return result;*/
}

void do_my_coin_flips(int myRank, int tt_per_rank, int tt_offset, int *coinFlips) {
	int i;
	for ( i = 0; i < tt_per_rank; i++ ) {
		coinFlips[tt_offset+i] = diceRoll(COIN_FLIP);
	}
}

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
			if (diceRoll(2) == 1) {
				ASSIGN_ATTACK(outboundEdge[i], troopsPerRival);
			} else {
				ASSIGN_DEFENSE(outboundEdge[i], troopsPerRival);
			}
		}
	}
}

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i, j, k;
	int buffer_switch = 0;

	srand( myRank +99999 ); //BIG COMMENT

	int *troopCounts; //Number of troops each territory has
	int *teamIDs; //What team is each territory under control of? (team ID = starting node # for now)
	int *coinFlips; //Used to determine who is doing what calculation each round

	//Everyone has their own slice of these arrays
	int **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge troop assignment data (passed around)
	EDGE_RESULT **edgeResults; //What was the result of the conflict between my territories and other territories?
	//int **occupations; //How many troops from team [COL] are on the border of team [ROW]?

	MPI_Status *statuses;
	MPI_Request *requests;
	
	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	//Rank 0 reads header information and sends to everybody else
	if (myRank == 0) {
		read_header_info( &tt_total );
	}

	//Use all-reduce to ensure all processes are aware of the total number of territories
	int temp_tt;
	MPI_Allreduce( &tt_total, &temp_tt, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	tt_total = temp_tt;

	//Now use the known total territory count to init other variables
	tt_per_rank = tt_total / commSize;
	
	troopCounts = calloc( tt_total, sizeof(int) );
	teamIDs = calloc ( tt_total, sizeof(int) );
	coinFlips = calloc ( tt_total, sizeof(int) );

	//TODO: Make adjMatrix a char**

	adjMatrix = calloc( tt_per_rank, sizeof(int *) );
	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	edgeResults = calloc( tt_per_rank, sizeof(EDGE_RESULT *) );

	for (i = 0; i < tt_per_rank; i++) {
		adjMatrix[i] = calloc( tt_total, sizeof(int) );
		edgeActivity[i] = calloc( tt_total, sizeof(int) );
		edgeResults[i] = calloc( tt_total, sizeof(EDGE_RESULT) ); 
	}

	//Create memory for our MPI Requests/Statuses
	statuses = calloc( 2, sizeof(MPI_Status) );
	requests = calloc( 2, sizeof(MPI_Request) );

	//Temp variables to enable all-reduce (might use these later when communicating further results)	
	int* troopCounts_temp = calloc( tt_total, sizeof(int) );
	int* teamIDs_temp = calloc( tt_total, sizeof(int) );
	int* coinFlips_temp = calloc( tt_total, sizeof(int) );

	tt_offset = read_from_file( tt_total, adjMatrix, troopCounts_temp, teamIDs_temp, myRank, commSize );
	do_my_coin_flips(myRank, tt_per_rank, tt_offset, coinFlips_temp);

	//Use all-reduce to ensure all processes are aware of initial team IDs and troop counts
	MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( coinFlips_temp, coinFlips, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

	// Print all territory current total troop counts
	if (myRank == 0) {
		printf("tt_total is %d\n", tt_total);
		printf("==============================================\n");
		printf("Current Total Troop Counts:\n");
		printf("==============================================\n");
		printf("TERRITORY | NUM_TROOPS\n");
		for (i = 0; i < tt_total; i++) {
			printf("%9d | %10d\n", i+tt_offset, troopCounts[i]);
		}
	}

	//MAIN LOOP (set up for one iteration for now)
	
	for( i = 0; i < tt_per_rank; i++ ) {
		//Apply a battle strategy for each node this Rank is responsible for
		apply_strategy( tt_offset+i, tt_total, troopCounts, teamIDs, adjMatrix[i], edgeActivity[i] );
	}


	//Graph printing

	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);
	if (myRank == 0) {
		printf("graph G {\n");
	}

	sleep(myRank + 1);
	for ( i = 0; i < tt_per_rank; i++ ) {
		int territoryID = tt_offset + i;
		//printf("Territory %d owned by Rank %d\n", territoryID, myRank);
		//printf("Territories bordered by territory %d:\n", territoryID);
		for ( j = 0; j < tt_total; j++ ) {
			if ( adjMatrix[i][j] == 1 ) {
				printf("\t\"%d{%d} (%d)\" -- \"%d{%d} (%d)\"", territoryID, teamIDs[territoryID], troopCounts[territoryID], j, teamIDs[j], troopCounts[j]);
				int numToPrint = edgeActivity[i][j];
				int posFlag = (numToPrint > 0);
				if (posFlag == 1) {
					printf(" [taillabel = \"%d\" fontcolor = \"red\"]\n", numToPrint);
				} else {
					numToPrint *= -1;
					printf(" [taillabel = \"%d\" fontcolor = \"blue\"]\n", numToPrint);
				}
			}
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);
	if (myRank == 0) {
		printf("\tsep = 1\n\toverlap = false\n\tsplines = true\n}\n");
	}


	sleep(2);

	MPI_Barrier( MPI_COMM_WORLD );

	//printf("[%d] Beginning result exchange\n", myRank);

	int *mpi_buffer[2];
	mpi_buffer[SEND] = calloc( tt_total * tt_per_rank, sizeof(int) );
	mpi_buffer[RECV] = calloc( tt_total * tt_per_rank, sizeof(int) );

	//LINEARIZE BUFFER DATA
	for ( j = 0; j < tt_per_rank; j++ ) {
		memcpy( mpi_buffer[SEND] + j*tt_total, edgeActivity[j], tt_total * sizeof(int) );
	}

	//Hot potato should: 

	// - Post SEND & RECV requests
	// - Work on the data in the SEND buffer
	// - Wait for SEND/RECV requests to complete for the next iteration of the I-loop
	// - Flip buffer switch ( SEND buffer becomes the RECV buffer [we're done with that data]; RECV becomes the SEND buffer )
	// - But don't send when i = commSize-1 because we don't need to

	//3 procs
	/*
	i = 0
	proc1 does data1
	proc2 does data2
	proc3 does data3
	SEND to i+1, RECV from i-1

	i = 1
	proc1 does data3
	proc2 does data1
	proc3 does data2
	SEND to i+1, RECV from i-1

	i = 1
	proc1 does data2
	proc2 does data3
	proc3 does data1
	SEND to i+1, RECV from i-1 <=== this one is only useful if we're RECVing into our Rank's actual data store (<spoiler>we're not.</spoiler>)

*/

	statuses[SEND].MPI_SOURCE = myRank;
	statuses[SEND].MPI_TAG = tt_offset;

	for ( i = 0; i < commSize; i++ ) { // I -> iteration of MPI hot-potato
		//Post requests if we need to
		if ( i < commSize - 1 ) {
			MPI_Isend( mpi_buffer[SEND], tt_total * tt_per_rank, MPI_INT, NEXT_RANK_FROM(myRank), statuses[SEND].MPI_TAG, MPI_COMM_WORLD, &requests[SEND] );
			MPI_Irecv( mpi_buffer[RECV], tt_total * tt_per_rank, MPI_INT, PREV_RANK_FROM(myRank), MPI_ANY_TAG, MPI_COMM_WORLD, &requests[RECV] );
		}
		//Work on data
		//printf("[%d] Working data from rank %d (Tag %d)\n", myRank, statuses[SEND].MPI_SOURCE, statuses[SEND].MPI_TAG);

		int working_offset = statuses[SEND].MPI_TAG;

		for ( j = 0; j < tt_per_rank; j++ ) { // J -> territories in currently received slice (rows of send buffer)
			for ( k = 0; k < tt_per_rank; k++ ) { // K -> my territories (columns of send buffer, also rows of my data)
				int my_tt_num = tt_offset + k;
				int other_tt_num = working_offset + j;

				if ( my_tt_num == other_tt_num ) { continue; } //Don't even consider the global diagonal
				//Do these two even border?
				if ( adjMatrix[ k ][ other_tt_num ] ) {
					int sameFlip = coinFlips[my_tt_num] == coinFlips[other_tt_num];
					int myNumLower = my_tt_num < other_tt_num;
					int isMyJob = 0;
					if (sameFlip) {
						if (myNumLower) { isMyJob = 1; }
					} else { //Different flips
						if (!myNumLower) { isMyJob = 1; }
					}

					if ( isMyJob ) {
						int myNumTroops = edgeActivity[k][other_tt_num];
						int otherNumTroops = ACC(mpi_buffer[SEND], j, my_tt_num);
					//	printf("[%d] Battle between MyTerr #%d and OtherTerr #%d is my job!\n", myRank, my_tt_num, other_tt_num);
					//printf("[%d] (T#%d) My Troops: %d; (T#%d) Other Troops: %d\n", 
							//myRank, my_tt_num, myNumTroops, other_tt_num, otherNumTroops);
						
						EDGE_RESULT result = do_battle( my_tt_num, other_tt_num, myNumTroops, otherNumTroops );
					//printf("[%d] AFTER: (T#%d) My Troops: %d; (T#%d) Other Troops: %d\n", 
					//	myRank, my_tt_num, result.myTroops, other_tt_num, result.otherTroops); 

						edgeResults[ k ][ other_tt_num ] = result;
					}
				}
			}
		}

		//Wait for requests
		MPI_Waitall( 2, requests, statuses );

		//Flip buffer switch
		buffer_switch = !buffer_switch;
	}

	//printf("[%d] HERE GOES FUCK ALL NOTHING\n", myRank);
	MPI_Barrier(MPI_COMM_WORLD);

	//Now that all of the edge results have been computed, we need to pass around potential occupation data.
	statuses[SEND].MPI_SOURCE = myRank;
	statuses[SEND].MPI_TAG = tt_offset;

	bzero( mpi_buffer[SEND], tt_total * tt_per_rank * sizeof(int) );
	bzero( mpi_buffer[RECV], tt_total * tt_per_rank * sizeof(int) );

	for ( i = 0; i < commSize; i++ ) { // I -> iteration of MPI hot-potato
		//Post requests if we need to
		//printf("[%d] GOIN AROUND FOR ITERATION %d\n", myRank, i);
		//if ( i < commSize - 1 ) {
			MPI_Irecv( mpi_buffer[RECV], tt_total * tt_per_rank, MPI_INT, PREV_RANK_FROM(myRank), MPI_ANY_TAG, MPI_COMM_WORLD, &requests[RECV] );
	//	}
		//printf("DONE\n");
		//iterate over slice we're given (report card for terr x)
		//on diag, continue

		//for spot [x][y]: check if current process has data for x
		// - if so, check to see if there's data in edgeResults[x][y]
		//    - anyone from terr [y] on border of [x]?

		//for spot [x][y]: check if current process has data for y
		// - if so, check for data in edgeResults[y][x]

		int working_offset = statuses[SEND].MPI_TAG;

		int x; //representing the current report card we have
		int y; //representing the territory the report card is asking about

		for(x = 0; x < tt_per_rank; x++) //for every report card we just recv'd...
		{
			for(y = 0; y < tt_total; y++) //for every territory the report card asks about...
			{
				int tt_card = x + working_offset;
				int localy = y - tt_offset;

				if(tt_card >= tt_offset && tt_card < tt_offset + tt_per_rank)
				{
					EDGE_RESULT e = edgeResults[x][y];
				
					//x = my
					//y = other
					if(e.myAction == DEFEND && e.otherAction == DEFEND)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += e.myTroops;
						ACC(mpi_buffer[SEND], x, y) += 0;
					}
					else if(e.myAction == ATTACK && e.otherAction == ATTACK)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += 0;
						ACC(mpi_buffer[SEND], x, y) += e.otherTroops;
					}
					else if(e.myAction == ATTACK)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += 0;
						ACC(mpi_buffer[SEND], x, y) += 0;
					}
					else if(e.otherAction == ATTACK)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += e.myTroops;
						ACC(mpi_buffer[SEND], x, y) += e.otherTroops;
					}
					
				}

				if(y >= tt_offset && y < tt_offset + tt_per_rank)
				{	
					EDGE_RESULT e = edgeResults[localy][tt_card];;
					
					//x is other
					//y is my
					if(e.myAction == DEFEND && e.otherAction == DEFEND)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += e.otherTroops;
						ACC(mpi_buffer[SEND], x, y) += 0;
					}
					else if(e.myAction == ATTACK && e.otherAction == ATTACK)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += 0;
						ACC(mpi_buffer[SEND], x, y) += e.myTroops;
					}
					else if(e.otherAction == ATTACK)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += 0;
						ACC(mpi_buffer[SEND], x, y) += 0;
					}
					else if(e.otherAction == DEFEND)
					{
						ACC(mpi_buffer[SEND], x, tt_card) += e.otherTroops;
						ACC(mpi_buffer[SEND], x, y) += e.myTroops;
					}
				}
			}
		}
		
		//if ( i < commSize - 1 ) {
			//Don't post a send until we're done filling in values
			MPI_Isend( mpi_buffer[SEND], tt_total * tt_per_rank, MPI_INT, NEXT_RANK_FROM(myRank), statuses[SEND].MPI_TAG, MPI_COMM_WORLD, &requests[SEND] );
	//	}

		//Wait for requests
		MPI_Waitall( 2, requests, statuses );

		//Flip buffer switch
		buffer_switch = !buffer_switch;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	for(i = 0; i < tt_per_rank; i++)
	{
		printf("[%d] Border Data for %d: ", myRank, i + tt_offset);
		for(j = 0; j < tt_total; j++)
		{
			printf("(%d): %d; ", j, ACC(mpi_buffer[SEND], i, j));
		}
		printf("\n");
	}

	bzero( teamIDs_temp, tt_total * sizeof(int) );
	bzero( troopCounts_temp, tt_total * sizeof(int) );

	for ( i = 0; i < tt_per_rank; i++ ) { // i -> my territory
		//For now, just find the max number in here and call the owner the winner.
		int maxNum = 0; int maxOwner = i+tt_offset;
		for ( j = 0; j < tt_total; j++ ) { // troops from terr j in terr i
			if ( ACC(mpi_buffer[SEND], i, j) > maxNum ) {
				maxNum = ACC(mpi_buffer[SEND], i, j);
				maxOwner = j;
			}
		}
		//Owner of territory i is now maxOwner (and the territory has maxNum troops)
		teamIDs_temp[tt_offset+i] = maxOwner;
		troopCounts_temp[tt_offset+i] = maxNum; 
	}

	MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

	//Graph printing

	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);
	if (myRank == 0) {
		printf("graph G {\n");
	}

	sleep(myRank + 1);
	for ( i = 0; i < tt_per_rank; i++ ) {
		int territoryID = tt_offset + i;
		//printf("Territory %d owned by Rank %d\n", territoryID, myRank);
		//printf("Territories bordered by territory %d:\n", territoryID);
		for ( j = 0; j < tt_total; j++ ) {
			if ( adjMatrix[i][j] == 1 ) {
				printf("\t\"%d{%d} (%d)\" -- \"%d{%d} (%d)\"\n", territoryID, teamIDs[territoryID], troopCounts[territoryID], j, teamIDs[j], troopCounts[j]);
			}
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);
	if (myRank == 0) {
		printf("\tsep = 1\n\toverlap = false\n\tsplines = true\n}\n");
	}


	sleep(2);

	MPI_Barrier( MPI_COMM_WORLD );

	MPI_Finalize();
	return EXIT_SUCCESS;
}
