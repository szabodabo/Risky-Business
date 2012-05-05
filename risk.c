#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define ASSIGN_ATTACK(E, NUM) (E = NUM)
#define ASSIGN_DEFENSE(E, NUM) (E = (-1) * NUM)
#define I_HAVE(TERR) (TERR > tt_offset && TERR < (tt_offset + tt_per_rank))
#define NEXT_RANK_FROM(R) (R+1 == commSize ? 0 : R+1)
#define PREV_RANK_FROM(R) ((R == 0) ? commSize-1 : R-1)
#define RECV 0
#define SEND 1
#define HEADS 1
#define TAILS 2
#define COIN_FLIP 2
#define ACC(A, R, C) A[tt_total * R + C]

#include "risk_input.h"

int tt_per_rank = 0; //Number of territories per MPI Rank
int tt_offset = 0; //Which territory do we start with? (which is the first territory in our set)
int tt_total = 0; //Total number of territories being warred

typedef enum answer_t {
	YES, NO
} ANSWER;

int diceRoll( int sides ) {
	int randInt = rand() % sides;
	//printf("DiceRoll: %d\n", randInt+1);
	return randInt + 1;
}

int do_battle(int teamA, int teamB)
{
	int a_roll = rand() % (teamA + 1); // 0 to 5
	int b_roll = rand() % (teamB + 1); // 0 to 4
	
	return a_roll - b_roll;
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

	srand(2); //BIG COMMENT

	//Everyone has each of these arrays in FULL
	int *troopCounts; //Number of troops each territory has
	int *teamIDs; //What team is each territory under control of? (team ID = starting node # for now)

	//Everyone has their own slice of these arrays
	int **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge data (passed around)
	int **myBattleFlips;

	//MPI_Datatype MATRIX_ROW;
	MPI_Request *requests;
	MPI_Status *statuses;

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	if (myRank == 0) {
		printf("Commsize is %d\n", commSize);
	}


	//Rank 0 reads header information and sends to everybody else
	if (myRank == 0) {
		read_header_info( &tt_total );
	}

	//MPI_Type_contiguous( tt_total, MPI_INT, &MATRIX_ROW );

	typedef struct battle_calc_msg_t {
		int source_rank;
		int row_offset;
		int *adjSlice;
		int *edgeSlice;
		int *battleFlip;
		int *edgeResult;
	} BATTLE_CALC_MSG;

	int SLICE_LEN = tt_per_rank * tt_total;

	int BCMSG_LENGTHS[6] = {
		1,
		1,
		SLICE_LEN,
		SLICE_LEN,
		SLICE_LEN,
		SLICE_LEN
	};

	MPI_Aint BCMSG_OFFSETS[6] = {
		0,
		sizeof(int), //Size of first element...
		2 * sizeof(int), //... + Size of second element...
		2 * sizeof(int) + SLICE_LEN * sizeof(int),
		2 * sizeof(int) + 2 * SLICE_LEN * sizeof(int),
		2 * sizeof(int) + 3 * SLICE_LEN * sizeof(int)
	};

	MPI_Datatype BCMSG_TYPES[6] = {
		MPI_INT,
		MPI_INT,
		MPI_INT,
		MPI_INT,
		MPI_INT,
		MPI_INT
	};

	MPI_Datatype MPI_BCMSG;
	MPI_Type_create_struct( 6, BCMSG_LENGTHS, BCMSG_OFFSETS, BCMSG_TYPES, &MPI_BCMSG );
	MPI_Type_commit( &MPI_BCMSG );


	//Use all-reduce to ensure all processes are aware of the total number of territories
	int temp_tt;
	MPI_Allreduce( &tt_total, &temp_tt, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	tt_total = temp_tt;
	
	//Now use the known total territory count to init other variables
	tt_per_rank = tt_total / commSize;

	requests = calloc( 2, sizeof(MPI_Request) );
	statuses = calloc( 2, sizeof(MPI_Status) );
	
	troopCounts = calloc( tt_total, sizeof(int) );
	teamIDs = calloc ( tt_total, sizeof(int) );

	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	adjMatrix = calloc( tt_per_rank, sizeof(int *) );
	myBattleFlips = calloc( tt_per_rank, sizeof(int *) );


	for (i = 0; i < tt_per_rank; i++) {
		edgeActivity[i] = calloc( tt_total, sizeof(int) );
		adjMatrix[i] = calloc( tt_total, sizeof(int) );
		myBattleFlips[i] = calloc( tt_total, sizeof(int) );
	}

	//Temp variables to enable all-reduce (might use these later when communicating further results)	
	int* troopCounts_temp = calloc( tt_total, sizeof(int) );
	int* teamIDs_temp = calloc( tt_total, sizeof(int) );
	
	tt_offset = read_from_file( tt_total, adjMatrix, troopCounts_temp, teamIDs_temp, myRank, commSize );

	//Use all-reduce to ensure all processes are aware of initial team IDs and troop counts
	MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	
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

	printf("MPI Rank %d initalized\n", myRank);
	
	/*
	sleep(myRank + 1);
	//DEBUG
	for(i = 0; i < tt_per_rank; i++) {
		printf("TT %d : ", i+tt_offset);
		for(j = 0; j < tt_total; j++) {
			printf("%d ", adjMatrix[i][j]);
		}
		printf("\n");
	}
	*/

	//MAIN LOOP (set up for one iteration for now)
	
	for( i = 0; i < tt_per_rank; i++ ) {
		//Apply a battle strategy for each node this Rank is responsible for
		apply_strategy( tt_offset+i, tt_total, troopCounts, teamIDs, adjMatrix[i], edgeActivity[i] );
	}

/*
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
				printf("\t\"%d (%d)\" -- \"%d (%d)\"", territoryID, troopCounts[territoryID], j, troopCounts[j]);
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
	*/

	MPI_Barrier( MPI_COMM_WORLD );

	printf("[%d] Beginning result exchange\n", myRank);
	BATTLE_CALC_MSG *recvBuffer = calloc( 1, sizeof(BATTLE_CALC_MSG) );
	BATTLE_CALC_MSG *sendBuffer = calloc( 1, sizeof(BATTLE_CALC_MSG) );

	//Allocate memory for send/receive buffers
	for ( i = 0; i < tt_per_rank; i++ ) {
		recvBuffer->adjSlice = calloc( tt_total * tt_per_rank, sizeof(int) );
		recvBuffer->edgeSlice = calloc( tt_total * tt_per_rank, sizeof(int) );
		recvBuffer->battleFlip = calloc( tt_total * tt_per_rank, sizeof(int) );
		recvBuffer->edgeResult = calloc( tt_total * tt_per_rank, sizeof(int) );

		sendBuffer->adjSlice = calloc( tt_total * tt_per_rank, sizeof(int) );
		sendBuffer->edgeSlice = calloc( tt_total * tt_per_rank, sizeof(int) );
		sendBuffer->battleFlip = calloc( tt_total * tt_per_rank, sizeof(int) );
		sendBuffer->edgeResult = calloc( tt_total * tt_per_rank, sizeof(int) );
	}

	recvBuffer->source_rank = myRank;
	recvBuffer->row_offset = tt_offset;

	for ( i = 0; i < tt_per_rank; i++ ) {
		for ( j = 0; j < tt_total; j++ ) {
			
			/* ==================== COIN FLIP ===================
			To decide which territory's rank computes the result on an edge:

			If the flips are the same, the territory with the lower ID 
			  is responsible for the calculation.
			If the flips are different, the terrirory with the higher ID
			  is responsible for the calculation.
			This should evenly distribute the work among the ranks and should
			  solve the lower-IDed-territories-have-more-work problem. 
			There might be a more efficient way to to this, 
			  but this seemed pretty good when I was working on it.  -DJS    */

			myBattleFlips[i][j] = diceRoll(COIN_FLIP);
			//printf("[%d] MyBattleFlips[%d][%d]: %d\n", myRank, i, j, myBattleFlips[i][j]);
		}
	}

	for ( j = 0; j < tt_per_rank; j++ ) {
		memcpy( recvBuffer->adjSlice + j * tt_total, adjMatrix[j], tt_total * sizeof(int) );
		memcpy( recvBuffer->edgeSlice + j * tt_total, edgeActivity[j], tt_total * sizeof(int) );
		memcpy( recvBuffer->battleFlip + j * tt_total, myBattleFlips[j], tt_total * sizeof(int) );
	}

	/*for ( i = 0; i < tt_per_rank; i++ ) {
		for ( j = 0; j < tt_total; j++ ) {
			
		}
	}*/

	//printf("[%d] RecvBuffer Populated!\n", myRank);

	for ( i = 0; i < commSize; i++ ) { // I -> iteration of MPI hot-potato

		for ( j = 0; j < tt_per_rank; j++ ) {
			for ( k = 0; k < tt_total; k++ ) {
				printf("[%d] (T %d -- T %d ) RecvBufAdjSlice[%d][%d] = %d\n", myRank, j+recvBuffer->row_offset, k, j, k, ACC(recvBuffer->adjSlice, j, k));
			}
		}

		for ( j = 0; j < tt_per_rank; j++ ) { // J -> my territories
			for ( k = 0; k < tt_per_rank; k++ ) { // K -> territories in currently received slice

				//Don't even bother if we're on the diagonal
				if ( tt_offset+j == recvBuffer->row_offset+k ) { continue; }

				//printf("[%d] RecvBattleFlips[%d][%d]: %d\n", myRank, i, j, recvBuffer->battleFlip[k][j]);

				printf("[%d] I [terr %d] flipped %d; Foe [terr %d] (rank %d) flipped %d\n", myRank, tt_offset+j, myBattleFlips[ j ][ recvBuffer->row_offset+k ], recvBuffer->row_offset+k, recvBuffer->source_rank, ACC(recvBuffer->battleFlip, k, tt_offset+j) );
				char sameFlip = ACC(recvBuffer->battleFlip, k, tt_offset+j) == myBattleFlips[j][recvBuffer->row_offset+k];
				char iAmHigher = tt_offset+j > recvBuffer->row_offset+k;
				char isMyJob;
				if (sameFlip == 1) {
					if (iAmHigher == 0) {
						isMyJob = 1;
					} else {
						isMyJob = 0;
					}
				} else { //Different Flips
					if (iAmHigher == 1) {
						isMyJob = 1;
					} else {
						isMyJob = 0;
					}
				}
				printf("[%d] J: %d; J-Offset: %d; K: %d\n", myRank, j, tt_offset, k);
				printf( "[%d] My Job: %d; Borders: %d\n", myRank, isMyJob, ACC(recvBuffer->adjSlice, k, tt_offset+j) );

				//If there's an adjacency, I'll decide the battle if it's my job to do so
				if ( ACC(recvBuffer->adjSlice, k, tt_offset+j) == 1 && isMyJob == 1 ) {
					
					int myNode_power = edgeActivity[ j ][ recvBuffer->row_offset+k ];
					int otherNode_power = ACC(recvBuffer->edgeSlice, k, tt_offset+j);

					int result = do_battle( myNode_power, otherNode_power );

					//IF RESULT < 0:
					// otherNode won (with *-result* troops surviving the attack)
					// but otherNode will see this as a win, so we'll store
					// it as a positive value (negating it)
					//IF RESULT > 0:
					// myNode won (with *result* troops surviving the attack)
					// but this is the otherNode's slot, so we'll negate it
					ACC(recvBuffer->edgeResult, k, tt_offset+j) = -1 * result;
					printf("[%d] Battle between MyNode %d and OtherNode %d: %d\n", myRank, tt_offset+j, recvBuffer->row_offset+k, result);					
				}
			}
		}

		printf("[%d] Boxing up rank %d's data...\n", myRank, recvBuffer->source_rank);
		//Information we've just finished working on is in recvBuffer.
		//Move the received info to the send buffer.
		sendBuffer->source_rank = recvBuffer->source_rank;
		sendBuffer->row_offset = recvBuffer->row_offset;

		for ( j = 0; j < tt_per_rank; j++ ) {
			memcpy( sendBuffer->adjSlice, recvBuffer->adjSlice, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->edgeSlice, recvBuffer->edgeSlice, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->battleFlip, recvBuffer->battleFlip, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->edgeResult, recvBuffer->edgeResult, tt_total * tt_per_rank * sizeof(int) );
		}
		MPI_Irecv( recvBuffer, 1, MPI_BCMSG, PREV_RANK_FROM(myRank), 0, MPI_COMM_WORLD, &requests[RECV] );
		MPI_Isend( sendBuffer, 1, MPI_BCMSG, NEXT_RANK_FROM(myRank), 0, MPI_COMM_WORLD, &requests[SEND] );
		MPI_Waitall( 2, requests, statuses );
		for ( j = 0; j < 2; j++ ) {
			printf("[%d] J=%d, Error = %d\n", myRank, j, statuses[j].MPI_ERROR);
		}

		printf("[%d] Completed Irecv of data from rank %d\n", myRank, recvBuffer->source_rank);


		for ( j = 0; j < tt_per_rank; j++ ) {
			for ( k = 0; k < tt_total; k++ ) {
				printf("[%d] (T %d -- T %d ) RecvBufAdjSlice[%d][%d] = %d\n", myRank, j+recvBuffer->row_offset, k, j, k, ACC(recvBuffer->adjSlice, j, k));
			}
		}
	}


	//Everyone calculates everything
	//Each edge calc done twice... take one of them

	//If one node is devastated, its conquerors this turn should fight over it

	//==================Teams and troop counts should be updated==============================

	//Temp variables to enable all-reduce (using these to communicate further results)	
	bzero( troopCounts_temp, tt_total * sizeof(int) );
	bzero( teamIDs_temp, tt_total * sizeof(int) );

	for ( i = 0; i < tt_total; i++ ) {
		//Set troopcount
		//Set teamids
	}
	
	//Use all-reduce to ensure all processes are aware of initial team IDs and troop counts
	MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	
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

	//===============================================================================================

	//Rinse and repeat


	MPI_Finalize();
	return EXIT_SUCCESS;
}
