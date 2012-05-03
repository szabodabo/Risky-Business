#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SEED 0
#define TRIALS 10
#define BATTLES 25
#define MAXTROOPS 100

void do_battle(int* teamA, int* teamB)
{
	double a = *teamA;
	double b = *teamB;

	int battles = a < b ? a : b;
	double afactor = a / b < 1 ? 0 : (a / b);
	printf("AFACTOR %f\n", afactor);
	double bfactor = b / a < 1 ? 0 : (b / a);
	printf("BFACTOR %f\n", bfactor);
	
	int i, j;
	for(i = 0; i < battles; i++)
	{
		double a_score = 0;
		j = 0;
		do
		{
			j++;
			a_score += j < afactor? (rand() % 6 + 1) : (rand() % 6 + 1) * (afactor - j);
		}
	       	while(j < afactor);

		double b_score = 0;
		j = 0;
		do
		{
			j++;
			b_score += j < bfactor? (rand() % 6 + 1) : (rand() % 6 + 1) * (bfactor - j);
		}
	       	while(j < afactor);
		
		(*teamA) -= (b_score >= a_score);
		(*teamB) -= (a_score >= b_score);
	}
}

void battle(int teamA, int teamB)
{
	int a = teamA;
	int b = teamB;

	printf("Battling A with %d troops versus B with %d troops...\n", teamA, teamB);
	do_battle(&a, &b);
	printf("Team A has %d troops left over\n", a);
	printf("Team B has %d troops left over\n", b);
	printf("\n");
}

int main()
{
	srand(SEED);
	int i;
	for(i = 0; i < BATTLES; i++)
	{
		battle(rand() % MAXTROOPS, rand() % MAXTROOPS);
	}

}


