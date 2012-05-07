#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../lib/risk_chance.h"

int main() {
	int nodes = 1000000; //1 million
	double degree = 4;

	int troopsPerNode = 50;
	int entropy = 20; // +- 20 troops per node (30 - 70)

	int maxDistance = 100; //A node may only connect with another node with id CURRENT_ID +- maxDistance.

	printf("%d\n", nodes);

	int i, j;

	for ( i = 0; i < nodes; i++ ) {
		int troops = troopsPerNode;
		int modifier = diceRoll( (entropy+1) * 2 ) - entropy;
		troops += modifier;

		printf("%d (%d):", i, troops);
		for ( j = i-maxDistance; j < i+maxDistance; j++ ) {
			if (j < 0 || j >= nodes) continue;

			//On average, connect to 4 other nodes.
			int bagSize = maxDistance*2;
			double connectChance = bagSize
		}
	}
	return EXIT_SUCCESS;
}

/*
10
0 (999): -1
1 (50): 0 -1
2 (50): 0 -1
3 (50): 0 -1
4 (50): 0 -1
5 (50): 0 -1
6 (50): 0 -1
7 (50): 0 -1
8 (50): 0 -1
9 (50): 0 -1
-1
*/