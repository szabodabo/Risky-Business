#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#include "risk_input.h"

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i;

	int total_tt = 0; //Total number of territories being warred
	int *troopCounts; //Number of troops each territory has
	int **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge data (passed around)
	int tt_per_rank; //Number of territories per MPI Rank
	int data_offset; //Offset for our territories

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
	MPI_Comm_size( MPI_COMM_WORLD, &commSize );

	printf("MPI Rank %d initalized\n", myRank);
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
	troopCounts = calloc( tt_per_rank, sizeof(int) );

	edgeActivity = calloc( tt_per_rank, sizeof(int *) );
	adjMatrix = calloc( tt_per_rank, sizeof(int *) );

	for (i = 0; i < tt_per_rank; i++) {
		edgeActivity[i] = calloc( total_tt, sizeof(int) );
		adjMatrix[i] = calloc( total_tt, sizeof(int) );
	}

	//Once we get here, allreduce has so nicely populated total_tt for everyone
	data_offset = read_from_file( total_tt, adjMatrix, troopCounts, myRank, commSize );

	printf("MPI Rank %d initalized\n", myRank);
	if (myRank == 0) {
		printf("total_tt is %d\n", total_tt);
	}

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

	MPI_Barrier(MPI_COMM_WORLD);

	if (myRank == 0) {
		printf("==============================================\n");
		printf("Current Total Troop Counts:\n");
		printf("==============================================\n");
		printf("TERRITORY | NUM_TROOPS\n");
	}
	sleep(myRank + 1);
	for (i = 0; i < tt_per_rank; i++) {
		printf("%9d | %10d\n", i+data_offset, troopCounts[i]);
	}


	MPI_Finalize();
	return EXIT_SUCCESS;
}
