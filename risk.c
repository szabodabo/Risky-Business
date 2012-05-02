#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mpi.h"

#define INPUT_BINARY "graphbin"
#define HEADER_BYTES 4

void read_header_info( int *total_tt )
{
	int fd = open(INPUT_BINARY, O_RDONLY);
	read(fd, total_tt, HEADER_BYTES);
	//DEBUG
	printf("HEADER: %d\n", *total_tt);
}

int read_from_file( int total_tt, int **adjMtx, int *troopCounts, int rank, int num_ranks ) {
	
	struct stat* stats = malloc(sizeof(struct stat));
	int success = stat(INPUT_BINARY, stats);

	if(success != 0)
		printf("WARNING: Couldn't stat %s (does it exist?)\n", INPUT_BINARY);
		
	int bytes_per_tt = (stats->st_size - HEADER_BYTES) / total_tt;
	int tt_per_rank = total_tt / num_ranks;
	
	int bytes = bytes_per_tt * tt_per_rank;
	int entries = bytes / sizeof(int);
	
	if(bytes * num_ranks != stats->st_size - HEADER_BYTES)
		printf("WARNING: Size of adjacency matrix must be a multiple of -np\n");
	
	MPI_File mfile;
	MPI_Status status;
	int* buffer = malloc(bytes);

	MPI_File_open(MPI_COMM_WORLD, INPUT_BINARY, MPI_MODE_RDONLY, MPI_INFO_NULL, &mfile);
	MPI_File_read_at(mfile, rank * bytes + HEADER_BYTES, buffer, entries, MPI_INT, &status);
	MPI_File_close(&mfile);

	int tt_a, tt_b;
	for(tt_a = 0; tt_a < tt_per_rank; tt_a++) {

		for(tt_b = 0; tt_b < total_tt; tt_b++) {
			adjMtx[tt_a][tt_b] = buffer[tt_a * total_tt + tt_b];
		}
		//Since no territory has an edge to itself, we used that spot in the binary file
		//to store the number of troops sitting on that territory. This pulls out that value
		//and sets the adjacency matrix to zero
		int index = rank * tt_per_rank + tt_a;
		troopCounts[tt_a] = adjMtx[tt_a][index];
		printf("%d : %d\n", rank, index);
		adjMtx[tt_a][index] = 0;
	}
	printf("HERE\n");
	free(stats);

	return rank * tt_per_rank;
}

int main( int argc, char **argv ) {
	int myRank, commSize;
	int i;

	int total_tt = 0; //Total number of territories being warred
	int *troopCounts; //Number of troops each territory has
	int **adjMatrix; //Adjacency matrix
	int **edgeActivity; //Edge data (passed around)
	int tt_per_rank; //Number of territories per MPI Rank

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
	read_from_file( total_tt, adjMatrix, troopCounts, myRank, commSize );

	printf("MPI Rank %d initalized\n", myRank);
	if (myRank == 0) {
		printf("total_tt is %d\n", total_tt);
	}

	sleep(myRank + 1);
	//DEBUG
	int j;
	for(i = 0; i < tt_per_rank; i++) {
		printf("TT %d : ", myRank);
		for(j = 0; j < total_tt; j++) {
			printf("%d ", adjMatrix[i][j]);
		}
		printf("\n");
	}	       

	if (myRank == 0) {
		printf("==============================================\n");
		printf("Current Total Troop Counts:\n");
		printf("==============================================\n");
		printf("TERRITORY | NUM_TROOPS\n");
		for (i = 0; i < total_tt; i++) {
			printf("%9d | %10d\n", i, troopCounts[i]);
		}
	}


	MPI_Finalize();
	return EXIT_SUCCESS;
}
