BG_COMPILE = mpixlc -I.
KR_COMPILE = mpicc -Wall -g -I.

bluegene: clean
	$(BG_COMPILE) risk.c -o risk
	$(BG_COMPILE) extras/generator.c -o bin/gen
kratos: 
	$(KR_COMPILE) risk.c -o bin/risk
	$(KR_COMPILE) extras/generator.c -o bin/gen
clean:
	rm -f stdout*
	rm -f stderr*
	rm -f core.*
	rm -f risk
	rm -f gen
	rm -f rand

