#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int** make_graph_complete(int tt_total, int *troopCounts)
{
	int** adjMatrix = calloc(tt_total, sizeof(int*));

	int i, j;
	for(i = 0; i < tt_total; i++)
	{
		adjMatrix[i] = calloc(tt_total, sizeof(int));
		adjMatrix[i][i] = troopCounts[i];
	}

	for(i = 0; i < tt_total; i++)
	{
		for(j = 0; j < tt_total; j++)
		{
			if(i != j)
			{
				//all edges are undirected
				adjMatrix[i][j] = 1;
				adjMatrix[j][i] = 1;
			}
		}
	}

	return adjMatrix;
}

int** make_graph_star(int tt_total, int* troopCounts)
{
	int** adjMatrix = calloc(tt_total, sizeof(int*));

	int i;
	for(i = 0; i < tt_total; i++)
	{
		adjMatrix[i] = calloc(tt_total, sizeof(int));
		adjMatrix[i][i] = troopCounts[i];
	}

	//0 is the center of the star
	for(i = 1; i < tt_total; i++)
	{
		//all edges are undirected
		adjMatrix[0][i] = 1;
		adjMatrix[i][0] = 1;
	}

	return adjMatrix;
}

int** make_graph_wheel(int tt_total, int* troopCounts)
{
	int** adjMatrix = make_graph_star(tt_total, troopCounts);

	int i, next;
	for(i = 1; i < tt_total; i++)
	{
		if(i != tt_total - 1)
			next = i + 1;
		else
			next = 1;
		
		adjMatrix[i][next] = 1;
		adjMatrix[next][i] = 1;
	}

	return adjMatrix;
}

int** make_graph_file(FILE *file, int* tt_total_out)
{
	int tt_total;
	fscanf(file, "%d", &tt_total);

	//used to communicate total territories
	*tt_total_out = tt_total;

	int **adjMatrix = calloc(tt_total, sizeof(int*));
		
	int i;
	for(i = 0; i < tt_total; i++)
		adjMatrix[i] = calloc(tt_total, sizeof(int));

	int tt_cur, troops;
	fscanf(file, "%d (%d):", &tt_cur, &troops);

	while(tt_cur != -1)
	{
		if(tt_cur >= tt_total)
		{
			printf("ERROR: Territory %d doesn't exist (graph is of size %d)\n", tt_cur, tt_total);
			break;
		}

		//Read to find the node the parent has an edge to
		int tt_edge;
		fscanf(file, "%d", &tt_edge);
		while(tt_edge != -1)
		{
			if(tt_edge >= tt_total)
			{
				printf("ERROR: Territory %d doesn't exist (graph is of size %d)\n", tt_edge, tt_total);
				break;
			}
			
			//edges are automatically bidirectional
			adjMatrix[tt_cur][tt_edge] = 1;
			adjMatrix[tt_edge][tt_cur] = 1;
			fscanf(file, "%d", &tt_edge);
		}
		
		adjMatrix[tt_cur][tt_cur] = troops;	
		fscanf(file, "%d (%d):", &tt_cur, &troops);
	}

	return adjMatrix;
}

void write_output(FILE *file, int tt_total, int **adjMatrix)
{
	fwrite(&tt_total, sizeof(int), 1, file);
	int i;
	for(i = 0; i < tt_total; i++)
	{
		fwrite(adjMatrix[i], sizeof(int), tt_total, file);
	}
}

int main(int argc, char* argv[])
{
	/*Graph creation w/out file 
		argv[1] - complete/star/etc.
		argv[2] - territories
		argv[3] - max troops per territory
		argv[4] - output file name
	*/
	if(argc == 5)
	{
		char* type = argv[1];
		int tt_total = atoi(argv[2]);
		int maxTroops = atoi(argv[3]);
		FILE* file = fopen(argv[4], "w");

		int* troopCounts = calloc(tt_total, sizeof(int)); //DEBUG AND TEMPORARY
		int i;
		for(i = 0; i < tt_total; i++)
			troopCounts[i] = maxTroops;

		int** adjMatrix;

		if(strcmp(type, "complete") == 0)
		{
			adjMatrix = make_graph_complete(tt_total, troopCounts);
		}
		else if(strcmp(type, "star") == 0)
		{
			adjMatrix = make_graph_star(tt_total, troopCounts);
		}
		else if(strcmp(type, "wheel") == 0)
		{
			adjMatrix = make_graph_wheel(tt_total, troopCounts);
		}
		else
		{
			printf("ERROR: Not a supported graph type");
			return EXIT_FAILURE;
		}

		write_output(file, tt_total, adjMatrix);
	}
	/* Graph creation from file
		argv[1] - input file
		argv[2] - output file
	*/
	else if(argc == 3)
	{
		struct stat st;
		int success = stat(argv[1], &st);

		if(success == 0)
		{
			FILE* infile = fopen(argv[1], "r");
			FILE* outfile = fopen(argv[2], "w");
			int tt_total = 0;

			int** adjMatrix = make_graph_file(infile, &tt_total);
			write_output(outfile, tt_total, adjMatrix);
		}
		else
		{
			printf("ERROR: Could not find input file");
			return EXIT_FAILURE;
		}
	}
	/* Graph verifier
		argv[1] - graph binary to be verified
	*/
	else if(argc == 2)
	{
		struct stat st;
		int success = stat(argv[1], &st);

		if(success == 0)
		{
			int fd = open(argv[1], O_RDONLY);
			int size;
			read(fd, &size, sizeof(int));
			int* buf = calloc(size * size, sizeof(int));
			read(fd, buf, st.st_size);

			int i;
			for(i = 0; i < size * size; i++)
			{
				if(i != 0 && i % size == 0)
					printf("\n");
				printf("%d ", buf[i]);
			}
			printf("\n");
		}
		else
		{
			printf("ERROR: Could not find input file");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
