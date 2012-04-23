BG_COMPILE = mpixlc -I.
KR_COMPILE = mpicc -Wall -g -I.

bluegene: clean
	$(BG_COMPILE) risk.c -o risk
kratos: 
	$(KR_COMPILE) risk.c -o risk
clean:
	rm -f stdout*
	rm -f stderr*
	rm -f core.*
	rm -f risk
