#include <stdio.h>
#include <sys/timeb.h>
#include <stdlib.h>

int main() {
	struct timeb tp;
	ftime( &tp );
	srand48( tp.millitm ); // an excellent seed

	double rand_dbl;
	int rand_int;

	int leftBound = -10;
	int rightBound = 10;

	int sum = 0;
	int count = 5;

	int i;
	for (i = 0; i < count; i++) {
		rand_dbl = drand48();
		rand_int = (rand_dbl * (rightBound - leftBound+1)) + leftBound;
		sum += rand_int;
	}

	int average = sum / count;
	printf("%d\n", average);

	return EXIT_SUCCESS;
}
