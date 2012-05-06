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

	int rc, n, i;

	char *fileName = argv[1];
	FILE *file;

	file = fopen( fileName, 'rb' );

	int numTerritories;

	n = fread( &numTerritories, sizeof(int), 1, file );

	for ( i = 0; i < numTerritories; i++ ) {
		int 
	}

	return EXIT_SUCCESS;
}