#ifndef RISK_INPUT_H
#define RISK_INPUT_H

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mpi.h"

#include "risk_macros.h"

void read_header_info( int *total_tt )
{
	int fd = open(INPUT_BINARY, O_RDONLY);
	read(fd, total_tt, HEADER_BYTES);
}

int read_from_file( int total_tt, int **adjMtx, int *troopCounts, int* teamIDs, int rank, int num_ranks ) {

	struct stat* stats = malloc(sizeof(struct stat));
	int success = stat(INPUT_BINARY, stats);

	if(success != 0)
		printf("WARNING: Couldn't stat %s (does it exist?)\n", INPUT_BINARY);
		
	int bytes_per_line = (stats->st_size - HEADER_BYTES) / total_tt;
	int tt_per_rank = total_tt / num_ranks;
	
	int bytes = bytes_per_line * tt_per_rank;
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
		//printf("%d a: %d\n", rank, tt_a);
		for(tt_b = 0; tt_b < total_tt; tt_b++) {
			//printf("%d b: %d\n", rank, tt_b);
			adjMtx[tt_a][tt_b] = buffer[tt_a * total_tt + tt_b];
		}
		//Since no territory has an edge to itself, we used that spot in the binary file
		//to store the number of troops sitting on that territory. This pulls out that value
		//and sets the adjacency matrix to zero
		int index = rank * tt_per_rank + tt_a;
		troopCounts[index] = adjMtx[tt_a][index];
		teamIDs[index] = index;
		adjMtx[tt_a][index] = 0;
	}
	free(stats);

	return rank * tt_per_rank;
}

#endif