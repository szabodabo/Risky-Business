//NumTerritories:
	//terrNUm, troopcount, team
	//Numterritories entries:
		// border: [0,1], numtroopsonedge, action[0,1]


#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv ) {
	if ( argc != 2 ) {
		fprintf( stderr, "USAGE: %s <generated binary result file>\n", argv[0] );
		exit( EXIT_FAILURE );
	}

	int i, j;

	char *fileName = argv[1];
	FILE *file;

	file = fopen( fileName, "rb" );

	int numTerritories;

	printf("graph G {\n");

	fread( &numTerritories, sizeof(int), 1, file );

	for ( i = 0; i < numTerritories; i++ ) {
		int terrNum, totalCount, teamNum;

		fread( &terrNum, sizeof(int), 1, file );
		fread( &totalCount, sizeof(int), 1, file );
		fread( &teamNum, sizeof(int), 1, file );

		printf("\t%d [label = \"%d{%d} (%d)\"]\n",
			terrNum, terrNum, teamNum, totalCount);

		for ( j = 0; j < numTerritories; j++ ) {
			int border; //0 -> No; 1 -> Yes
			int numTroopsOnEdge; //How many troops stationed?
			int action; //0 -> Def; 1 -> Att

			fread( &border, sizeof(int), 1, file );
			fread( &numTroopsOnEdge, sizeof(int), 1, file );
			fread( &action, sizeof(int), 1, file );

			if ( border == 1 ) {
				//"9{9} (0)" -- "0{0} (273)" [taillabel = "0" fontcolor = "blue"]
				if ( action == 0 ) {
					printf("\t\t%d -- %d [taillabel = \"%d\" fontcolor = \"blue\"]\n",
						i, j, numTroopsOnEdge);
				} else {
					printf("\t\t%d -- %d [taillabel = \"%d\" fontcolor = \"red\"]\n",
						i, j, numTroopsOnEdge);			
				}
			}
		}
	}
	
	printf("\tsep = 1\n\toverlap = false\n\tsplines = true\n");

	printf("}\n");

	return EXIT_SUCCESS;
}