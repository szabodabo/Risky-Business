#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#define NUM_TERRITORIES 40

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i;

	int *troopCounts; //Number of troops each territory has
	int **edgeActivity; //Edge data (passed around)
	int tt_per_rank; //Number of territories per MPI Rank

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	tt_per_rank = NUM_TERRITORIES / commSize;
	troopCounts = calloc( NUM_TERRITORIES, sizeof(int) );

	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	for (i = 0; i < tt_per_rank; i++) {
		edgeActivity[i] = calloc( NUM_TERRITORIES, sizeof(int) );
	}



	printf("MPI Rank %d initalized\n", myRank);
	if (myRank == 0) {
		printf("Commsize is %d\n", commSize);
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}
