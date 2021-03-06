#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include "mpi.h"

#include "lib/risk_macros.h"
#include "lib/risk_chance.h"
#include "lib/risk_input.h"
#include "lib/risk_battles.h"
#include "lib/risk_output.h"
#include "lib/risk_performance.h"

//GIANT IDEA:
//Rule change --> it is totally legal to not leave any troops in your home node. If all troops from a node
//get killed while away and nobody is left home, that's fine. That node just has 0 troops to allocate during
//every phase and it will get conquered pretty damn fast (battle resolution should report no troops lost if a team
//fights an army with 0 troops)

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i, j, k;
	int buffer_switch = 0;

	int tt_per_rank = 0; //Number of territories per MPI Rank
	int tt_offset = 0; //Which territory do we start with? (which is the first territory in our set)
	int tt_total = 0; //Total number of territories being warred
	int num_iterations = 0;
	int max_iterations = INT_MAX;

	//Timing metrics:
	double global_start = 0, global_end = 0;
	double phase1_start = 0, phase1_end = 0;
	double phase2_start = 0, phase2_end = 0;
	double phase1_total = 0, phase2_total = 0; // Number of seconds elapsed
	//sanity check!
	double total_seconds = rdtsc();

	if ( argc == 2 && strncmp(argv[1], "--max-iterations=", strlen("--max-iterations=")) == 0 ) {
		sscanf(argv[1], "--max-iterations=%d", &max_iterations);
	}

	srand( myRank ); //BIG COMMENT -- NEED TO ADD BACK time(NULL) after testing

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

	//TODO: Make adjMatrix a char** maybe?

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

	global_start = rdtsc();

	//Use all-reduce to ensure all processes are aware of initial team IDs and troop counts
	MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
	MPI_Allreduce( coinFlips_temp, coinFlips, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

	int *mpi_buffer[2];
	mpi_buffer[SEND] = calloc( tt_total * tt_per_rank, sizeof(int) );
	mpi_buffer[RECV] = calloc( tt_total * tt_per_rank, sizeof(int) );

	//MAIN LOOP 

	int loopCondition = 1;

	while ( loopCondition == 1 ) {

		phase1_start = rdtsc();

		num_iterations++;

		for ( i = 0; i < tt_per_rank; i++ ) {
			bzero( edgeActivity[i], tt_total * sizeof(int) );
			bzero( edgeResults[i], tt_total * sizeof(EDGE_RESULT) );

		}
	
		for( i = 0; i < tt_per_rank; i++ ) {
			//Apply a battle strategy for each node this Rank is responsible for
			apply_strategy( tt_offset+i, tt_total, troopCounts, teamIDs, adjMatrix[i], edgeActivity[i] );
		}
		
		//Graph printing

		MPI_Barrier(MPI_COMM_WORLD);

		print_graph(num_iterations, myRank, commSize, tt_per_rank, 
			adjMatrix, edgeActivity, troopCounts, teamIDs);

		MPI_Barrier( MPI_COMM_WORLD );

		//printf("[%d] Beginning result exchange\n", myRank);

		bzero(mpi_buffer[SEND], tt_total * tt_per_rank * sizeof(int) );
		bzero(mpi_buffer[RECV], tt_total * tt_per_rank * sizeof(int) );

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


		statuses[SEND].MPI_SOURCE = myRank;
		statuses[SEND].MPI_TAG = tt_offset;


		for ( i = 0; i < commSize; i++ ) { // I -> iteration of MPI hot-potato
			//Post requests if we need to
			if ( i < commSize - 1 ) {
				MPI_Isend( mpi_buffer[SEND], tt_total * tt_per_rank, MPI_INT, NEXT_RANK_FROM(myRank), statuses[SEND].MPI_TAG, MPI_COMM_WORLD, &requests[SEND] );
				MPI_Irecv( mpi_buffer[RECV], tt_total * tt_per_rank, MPI_INT, PREV_RANK_FROM(myRank), MPI_ANY_TAG, MPI_COMM_WORLD, &requests[RECV] );
			}
			
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
							
							//printf("[%d] (T#%d) My Troops: %d; (T#%d) Other Troops: %d\n", 
							//	myRank, my_tt_num, myNumTroops, other_tt_num, otherNumTroops);
							
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

		phase1_end = rdtsc();

		MPI_Barrier( MPI_COMM_WORLD );

		phase2_start = rdtsc();

		//Now that all of the edge results have been computed, we need to pass around potential occupation data.
		statuses[SEND].MPI_SOURCE = myRank;
		statuses[SEND].MPI_TAG = tt_offset;

		bzero( mpi_buffer[SEND], tt_total * tt_per_rank * sizeof(int) );
		bzero( mpi_buffer[RECV], tt_total * tt_per_rank * sizeof(int) );

		for ( i = 0; i < commSize; i++ ) { // I -> iteration of MPI hot-potato
			MPI_Irecv( mpi_buffer[RECV], tt_total * tt_per_rank, MPI_INT, PREV_RANK_FROM(myRank), MPI_ANY_TAG, MPI_COMM_WORLD, &requests[RECV] );

			int working_offset = statuses[SEND].MPI_TAG;

			int x; //representing the current report card we have
			int y; //representing the territory the report card is asking about

			for( x = 0; x < tt_per_rank; x++ ) { //for every report card we just recv'd...
				for( y = 0; y < tt_total; y++ ) { //for every territory the report card asks about...
					
					int tt_card = x + working_offset;

					/* report card for tt_card wants to know about the conflict between tt_card and y

						you can answer this question if you have data for tt_card or data for y

						if you have data for tt_card:

						look up conflict [x][y] --> this spits back data
						--myAction, myTroops == tt_card's actions
						--otherAction, otherTroops == y's actions

						write on the card -
						--troops on tt_card edge from tt_card - write in card[x][tt_card] (myTroops)
						--troops on tt_card edge from y - write in card[x][y] (otherTroops)


						if you have data for y:

						look up conflict [y - tt_offset][tt_card] --> this spits back data
						--myAction, myTroops == y's actions
						--otherAction, otherTroops == tt_card's actions

						write on the card -
						--troops on tt_card edge from tt_card - write in card[x][tt_card] (otherTroops)
						--troops on tt_card edge from y - write in card[x][y] (myTroops)
					
					*/

					if( tt_card >= tt_offset && tt_card < tt_offset + tt_per_rank ) {
						EDGE_RESULT e = edgeResults[x][y];
					
						//the actions taken by the 'card' territory are MYACTION
						if( e.myAction == DEFEND && e.otherAction == DEFEND ) {
							ACC(mpi_buffer[SEND], x, tt_card) += e.myTroops;
							ACC(mpi_buffer[SEND], x, y) += 0;
						}
						else if( e.myAction == ATTACK && e.otherAction == ATTACK ) {
							ACC(mpi_buffer[SEND], x, tt_card) += 0;
							ACC(mpi_buffer[SEND], x, y) += e.otherTroops;
						}
						else if( e.myAction == ATTACK ) {
							ACC(mpi_buffer[SEND], x, tt_card) += 0;
							ACC(mpi_buffer[SEND], x, y) += 0;
						}
						else if( e.myAction == DEFEND ) {
							ACC(mpi_buffer[SEND], x, tt_card) += e.myTroops;
							ACC(mpi_buffer[SEND], x, y) += e.otherTroops;
						}
						
					}

					if(y >= tt_offset && y < tt_offset + tt_per_rank) {	
						EDGE_RESULT e = edgeResults[y - tt_offset][tt_card];

						//the actions taken by the card territory are OTHERACTION
						if(e.otherAction == DEFEND && e.myAction == DEFEND) {
							ACC(mpi_buffer[SEND], x, tt_card) += e.otherTroops;
							ACC(mpi_buffer[SEND], x, y) += 0;
						}
						else if(e.otherAction == ATTACK && e.myAction == ATTACK) {
							ACC(mpi_buffer[SEND], x, tt_card) += 0;
							ACC(mpi_buffer[SEND], x, y) += e.myTroops;
						}
						else if(e.otherAction == ATTACK) {
							ACC(mpi_buffer[SEND], x, tt_card) += 0;
							ACC(mpi_buffer[SEND], x, y) += 0;
						}
						else if(e.otherAction == DEFEND) {
							ACC(mpi_buffer[SEND], x, tt_card) += e.otherTroops;
							ACC(mpi_buffer[SEND], x, y) += e.myTroops;
						}
					}
				}
			}
			
			//Don't post a send until we're done filling in values
			MPI_Isend( mpi_buffer[SEND], tt_total * tt_per_rank, MPI_INT, NEXT_RANK_FROM(myRank), statuses[SEND].MPI_TAG, MPI_COMM_WORLD, &requests[SEND] );

			//Wait for requests
			MPI_Waitall( 2, requests, statuses );

			//Flip buffer switch
			buffer_switch = !buffer_switch;
		}

		MPI_Barrier( MPI_COMM_WORLD );

		/*
		for ( i = 0; i < tt_per_rank; i++ ) {
			printf("[%d - %d] Border Data for %d: ", myRank, num_iterations, i + tt_offset);
			for(j = 0; j < tt_total; j++)
			{
				printf("(%d): %d; ", j, ACC(mpi_buffer[SEND], i, j));
			}
			printf("\n\n");
		}*/

		bzero( teamIDs_temp, tt_total * sizeof(int) );
		bzero( troopCounts_temp, tt_total * sizeof(int) );
		bzero( coinFlips_temp, tt_total * sizeof(int) );

		//Resolve inter-node conflicts

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

		//Do coin flips
		do_my_coin_flips(myRank, tt_per_rank, tt_offset, coinFlips_temp);

		//If I'm bordered only by neighbors on my team, give my neighbors my troops.
		//The problem with this is that if a friendly neighbor gets taken over, I'm now screwed.
		//We can look at the teamIDs to find out who the neighbors are and send accordingly,
		// but this will require a second allreduce (first allreduce provides teams, so we'd have to wait)

		//Reinforcements: Let's give a 20% boost to each territory's troop count
		for ( i = 0; i < tt_per_rank; i++ ) {  // i -> for each of my territories
			int troops = troopCounts_temp[ tt_offset+i ];
			troops = (double) troops * 1.20;
			troopCounts_temp[ tt_offset+i ] = troops;
		}

		MPI_Allreduce( troopCounts_temp, troopCounts, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
		MPI_Allreduce( teamIDs_temp, teamIDs, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
		MPI_Allreduce( coinFlips_temp, coinFlips, tt_total, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

		//not sure if we need this. look at it later.
		MPI_Barrier( MPI_COMM_WORLD );

		//If one team controls all territories, stop the game.
		loopCondition = 0;
		int curTeam = teamIDs[0];

		for ( i = 1; i < tt_total; i++ ) {
			if ( curTeam != teamIDs[i] ) { 
				loopCondition = 1;
				break; 
			}
		}

		if ( loopCondition == 0 ) {
			if ( myRank == 0 ) {
				//sleep(1);
				printf("GAME OVER: Team %d wins after %d rounds.\n", curTeam, num_iterations);
			}
		}

		if ( loopCondition == 1 ) { //If already chose to end the game, we don't need to do this.

			loopCondition = 0;

			//If only one team has troops remaining, stop the game.
			int teamsWithTroops = 0;
			int lastTeam = -1;

			for ( i = 0; i < tt_total; i++ ) {
				if ( troopCounts[i] > 0 && teamIDs[i] != lastTeam ) {
					lastTeam = teamIDs[i];
					teamsWithTroops++;
				}
				if ( teamsWithTroops > 1 ) {
					loopCondition = 1;
					break;
				}
			} //If we fell through, that means the game's over.
			if ( loopCondition == 0 ) {
				if ( myRank == 0 ) {
					//sleep(1);
					printf("GAME OVER: Team %d wins after %d rounds.\n", lastTeam, num_iterations);
				}
			}
		}
		if ( loopCondition == 1 && num_iterations >= max_iterations ) {
			loopCondition = 0;
			if ( myRank == 0 ) {
				//sleep(1);
				printf("GAME OVER: Maximum number of iterations reached (MAX: %d)\n", max_iterations);
			}
		}

		phase2_end = rdtsc();

		phase1_total += (phase1_end - phase1_start) / CURRENT_CLOCK;
		phase2_total += (phase2_end - phase2_start) / CURRENT_CLOCK;
	}


	global_end = rdtsc();
	double global_total = (global_end - global_start) / CURRENT_CLOCK;

	global_total /= num_iterations;
	phase1_total /= num_iterations;
	phase2_total /= num_iterations;

	double global_avg;
	MPI_Allreduce(&global_total, &global_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	global_avg /= commSize;

	double phase1_avg;
	MPI_Allreduce(&phase1_total, &phase1_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	phase1_avg /= commSize;

	double phase2_avg;
	MPI_Allreduce(&phase2_total, &phase2_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	phase2_avg /= commSize;

	total_seconds = rdtsc() - total_seconds;
	total_seconds /= CURRENT_CLOCK;

	if(myRank == 0)
	{
		printf("#METRIC RANKS TERRITORIES SECONDS\n");
		printf("GLOBAL %d %d %f\n", commSize, tt_total, global_avg);
		printf("PHASE1 %d %d %f\n", commSize, tt_total, phase1_avg);
		printf("PHASE2 %d %d %f\n", commSize, tt_total, phase2_avg);
		printf("#TOTAL RUNTIME: %f seconds\n", total_seconds);
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}
