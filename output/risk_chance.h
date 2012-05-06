#ifndef RISK_CHANCE_H
#define RISK_CHANCE_H

#include <stdlib.h>
#include "risk_macros.h"

int diceRoll( int sides ) {
	int randInt = rand() % sides;
	return randInt + 1;
}

void do_my_coin_flips(int myRank, int tt_per_rank, int tt_offset, int *coinFlips) {
	int i;
	for ( i = 0; i < tt_per_rank; i++ ) {
		coinFlips[tt_offset+i] = diceRoll(COIN_FLIP);
	}
}

#endif