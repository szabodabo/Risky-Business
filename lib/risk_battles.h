#ifndef RISK_BATTLES_H
#define RISK_BATTLES_H

#include <stdlib.h>

#include "risk_macros.h"
#include "risk_chance.h"


typedef struct edge_result_t {
	int myTerr; //ID of territory A
	int otherTerr; //ID of territory B
	int myTroops; //Troops owned by A
	int otherTroops; //Troops owned by B
	int myAction; //Attack or defend 
	int otherAction; //Attack or defend
} EDGE_RESULT;

//Apply the strategy for team <teamID> on country <territory> by looking at the troop counts and controlling teams for 
//each node on the map. Use the adjacency matrix to figure out our neighbors.  Place results in edgeActivity
void apply_strategy(int territory, int tt_total, int *troopCounts, int *teamIDs, int *adjList, int *outboundEdge)
{
	//simple strategy to test and debug with - split troops evenly on all edges, all attacking
	//if there is a remainder of troops, don't do anything with them
	int myTroops = troopCounts[territory];
	int neighborCount = 0;
	int i;
	for( i = 0; i < tt_total; i++ ) {
		//don't need to attack my own team
		neighborCount += ( adjList[i] == 1 ) && ( teamIDs[i] != teamIDs[territory] );
	}

	if ( neighborCount == 0 ) {
		return;
	}

	int troopsPerRival = myTroops / neighborCount;
	int remainder = myTroops % neighborCount;
	int troopsToPlace;

	for( i = 0; i < tt_total; i++ ) {
		troopsToPlace = troopsPerRival;
		//printf("[T%d] Troops Per Rival: %d; MyTroops: %d, Neighbors: %d; remainder: %d\n", 
		//	territory, troopsPerRival, myTroops, neighborCount, remainder);
		
		if( adjList[i] == 1 && teamIDs[i] != teamIDs[territory] ) {
			if (remainder > 0) {
				troopsToPlace += remainder;
				remainder = 0;
			}
			
			if (diceRoll(2) == 1) {
				ASSIGN_ATTACK(outboundEdge[i], troopsToPlace);
			} else {
				ASSIGN_DEFENSE(outboundEdge[i], troopsToPlace);
			}
		}
	}
}

//If either argument is negative, that team is defending
EDGE_RESULT do_battle(int terrA, int terrB, int troopsA, int troopsB)
{
	EDGE_RESULT result;

	result.myTerr = terrA;
	result.otherTerr = terrB;

	int actionA = troopsA < 0 ? DEFEND : ATTACK;
	int actionB = troopsB < 0 ? DEFEND : ATTACK;

	troopsA = abs(troopsA);
	troopsB = abs(troopsB);

	result.myAction = actionA;
	result.otherAction = actionB;

	if(actionA == DEFEND && actionB == DEFEND)
	{
		//Do nothing --> left here in case we want to do something later
	}
	else
	{
		int i;
		int a_score = 0, b_score = 0;

		while(troopsA > 0 && troopsB > 0)
		{
			for(i = 0; i < troopsA; i++)
				a_score += rand() % 2 + (actionA == DEFEND) * (rand() % 2);
			for(i = 0; i < troopsB; i++)
				b_score += rand() % 2 + (actionB == DEFEND) * (rand() % 2);

			troopsA -= a_score;
			troopsB -= b_score;
		}

		troopsA = troopsA < 0 ? 0 : troopsA;
		troopsB = troopsB < 0 ? 0 : troopsB;
	}

	result.myTroops = troopsA;
	result.otherTroops = troopsB;

	return result;
}

#endif