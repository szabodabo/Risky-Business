#ifndef RISK_OUTPUT_H
#define RISK_OUTPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mpi.h"
#include "risk_macros.h"

void print_graph(int round_num, int rank, int num_ranks, int tt_per_rank, int **adjMatrix, int **edgeActivity, int* troopCounts, int *teamIDs)
{
	int digits = 0;
	int r = round_num;
	while(r > 0)
	{
		digits++;
		r /= 10;
	}

	char* graphout = malloc(12 + digits);
	sprintf(graphout, "graphs/graphout%d", round_num);

	MPI_File mfile;
	MPI_Status status;
	
	MPI_File_open(MPI_COMM_WORLD, graphout, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &mfile);

	int tt_total = num_ranks * tt_per_rank;
	int ints_per_tt 	= 3 + (tt_total * 3);
	int ints_per_rank 	= tt_per_rank * ints_per_tt;

	//write file header
	if(rank == 0)
	{
		int tt_total = tt_per_rank * num_ranks;
		MPI_File_write_at(mfile, 0, &tt_total, 1, MPI_INT, &status);
	}

	int offset = (ints_per_rank * rank + 1) * sizeof(int); //the extra 1 is the file header int
	int* buffer = calloc(ints_per_rank, sizeof(int));

	int tt_offset = rank * tt_per_rank;
	int i, j;
	for(i = 0; i < tt_per_rank; i++)
	{
		int buf_offset = ints_per_tt * i;
		buffer[buf_offset++] = tt_offset + i;
		buffer[buf_offset++] = troopCounts[tt_offset + i];
		buffer[buf_offset++] = teamIDs[tt_offset + i];

		for(j = 0; j < tt_total; j++)
		{
			buffer[buf_offset++] = adjMatrix[i][j];
			buffer[buf_offset++] = abs(edgeActivity[i][j]);
			buffer[buf_offset++] = edgeActivity[i][j] > 0;
		}
	}

	MPI_File_write_at(mfile, offset, buffer, ints_per_rank, MPI_INT, &status);

	MPI_File_close(&mfile);

	free(graphout);
	free(buffer);
}

#endif