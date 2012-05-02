#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	struct stat st;
	int success = stat(argv[1], &st);
	if(argc == 3 && success == 0)
	{
		FILE* infile = fopen(argv[1], "r");
		FILE* outfile = fopen(argv[2], "w");
		
		int nodes;
		fscanf(infile, "%d", &nodes);

		//BEGIN FILE READING
	
		int** nodeList = calloc(nodes, sizeof(int*));
		
		int i;
		for(i = 0; i < nodes; i++)
			nodeList[i] = calloc(nodes, sizeof(int));

		//Read to find the 'parent node'
		int curNode, troops;
		fscanf(infile, "%d (%d):", &curNode, &troops);
		while(curNode != -1)
		{
			if(curNode >= nodes)
			{
				printf("ERROR: Node %d doesn't exist (graph is of size %d)\n", curNode, nodes);
				return 1;
			}

			//Read to find the node the parent has an edge to
			int edgeNode;
			fscanf(infile, "%d", &edgeNode);
			while(edgeNode != -1)
			{
				if(edgeNode >= nodes)
				{
					printf("ERROR: Node %d doesn't exist (graph is of size %d)\n", edgeNode, nodes);
					return 1;
				}
				
				//edges are automatically bidirectional
				nodeList[curNode][edgeNode] = 1;
				nodeList[edgeNode][curNode] = 1;
				fscanf(infile, "%d", &edgeNode);
			}
			
			nodeList[curNode][curNode] = troops;	
			fscanf(infile, "%d (%d):", &curNode, &troops);
		}

		fwrite(&nodes, sizeof(int), 1, outfile);
		int a, b;
		for(a = 0; a < nodes; a++)
		{
			printf("%d (%d troops): ", a, nodeList[a][a]);
			for(b = 0; b < nodes; b++)
				printf("%d ", nodeList[a][b]);
			printf("\n");
			fwrite(nodeList[a], sizeof(int), nodes, outfile);
		}

	}
	else if(success == 0)
	{
		int fd = open(argv[1], O_RDONLY);
		int size;
		read(fd, &size, sizeof(int));
		int* buf = calloc(st.st_size / sizeof(int), sizeof(int));
		read(fd, buf, st.st_size);

		int i;
		for(i = 0; i < st.st_size / sizeof(int); i++)
		{
			if(i != 0 && i % size == 0)
				printf("\n");
			printf("%d ", buf[i]);
		}
	}

	return EXIT_SUCCESS;
}
