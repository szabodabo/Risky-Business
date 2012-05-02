#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#define NUM_TERRITORIES 40

void read_from_file( char **adjMtx, int *troopCounts, int rank, int num_ranks );
void read_header_info( int *totalNumTerr );

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i;

	int total_tt = 0; //Total number of territories being warred
	int *troopCounts; //Number of troops each territory has
	char **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge data (passed around)
	int tt_per_rank; //Number of territories per MPI Rank

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	printf("MPI Rank %d initalized\n", myRank);
	if (myRank == 0) {
		printf("Commsize is %d\n", commSize);
	}

	tt_per_rank = NUM_TERRITORIES / commSize;
	troopCounts = calloc( NUM_TERRITORIES, sizeof(int) );

	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	for (i = 0; i < tt_per_rank; i++) {
		edgeActivity[i] = calloc( NUM_TERRITORIES, sizeof(int) );
	}

	//Rank 0 reads header information and sends to everybody else
	if (myRank == 0) {
		read_header_info( &total_tt );
	}

	MPI_Allreduce( &total_tt, &total_tt, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

	//Once we get here, allreduce has so nicely populated total_tt for everyone
	read_from_file( adjMatrix, troopCounts, myRank, commSize );

	
	if (myRank == 0) {
		printf("==============================================\n");
		printf("Current Total Troop Counts:\n");
		printf("==============================================\n");
		printf("TERRITORY | NUM_TROOPS\n");
		for (i = 0; i < total_tt; i++) {
			printf("%9d | %10d\n", t, troopCounts[i]);
		}
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}
