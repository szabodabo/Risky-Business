#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SEED 0
#define TRIALS 10
#define BATTLES 100//0000
#define MAXTROOPS 1000

int do_battle(int teamA, int teamB)
{
	int a_roll = rand() % (teamA + 1); // 0 to 5
	int b_roll = rand() % (teamB + 1); // 0 to 4
	
	return a_roll - b_roll;
}

void battle(int teamA, int teamB)
{
	int a, b;
	int a_wins = 0;
	int b_wins = 0;
	int ties = 0;
	double a_tleft = 0;
	double b_tleft = 0;

	printf("Simulating %d battles, A with %d troops versus B with %d troops...\n", BATTLES, teamA, teamB);
	
	int i;
	for(i = 0; i < BATTLES; i++)
	{
		int result = do_battle(teamA, teamB);

		a_wins += result > 0;
		b_wins += result < 0;
		ties += result == 0;

		a_tleft += result > 0 ? result : 0;
		b_tleft += result < 0 ? -result : 0;
	}

	double aleft = a_tleft / a_wins;
	double bleft = b_tleft / b_wins;
	printf("A won %d battles and on average had %5.2lf percent of its army left after victory (%f troops)\n", a_wins, aleft / teamA * 100, floor(aleft));
	printf("B won %d battles and on average had %5.2lf percent of its army left after victory (%f troops)\n", b_wins, bleft / teamB * 100, floor(bleft));
	printf("\n");
}

int main()
{
	srand(time(NULL));

	battle(MAXTROOPS, MAXTROOPS);
	battle(MAXTROOPS, MAXTROOPS * .75);
	battle(MAXTROOPS, MAXTROOPS * .5);
	battle(MAXTROOPS, MAXTROOPS * .25);
}


