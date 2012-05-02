BG_COMPILE = mpixlc -I.
KR_COMPILE = mpicc -Wall -g -I.

bluegene: clean
	$(BG_COMPILE) risk.c -o risk
	$(BG_COMPILE) generator.c -o gen
kratos: 
	$(KR_COMPILE) risk.c -o risk
	$(KR_COMPILE) generator.c -o gen
clean:
	rm -f stdout*
	rm -f stderr*
	rm -f core.*
	rm -f risk
	rm -f gen

