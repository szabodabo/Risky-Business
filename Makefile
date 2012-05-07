BG_COMPILE = mpixlc -I.
KR_COMPILE = mpicc -Wall -g -I.

bluegene: clean
	$(BG_COMPILE) risk.c -o risk
	$(BG_COMPILE) extras/generator.c -o bin/gen
	$(BG_COMPILE) extras/to_graphviz.c -o bin/to_graphviz
	$(BG_COMPILE) extras/input_creator.c -o bin/input_creator
kratos: 
	$(KR_COMPILE) risk.c -o bin/risk
	$(KR_COMPILE) extras/generator.c -o bin/gen
	$(KR_COMPILE) extras/to_graphviz.c -o bin/to_graphviz
	$(KR_COMPILE) extras/input_creator.c -o bin/input_creator
clean:
	rm -f stdout*
	rm -f stderr*
	rm -f core.*
	rm -f bin/*
	rm -f graphs/graphviz/png/*.png
	rm -f graphs/graphviz/*.gv
	rm -f graphs/graphout*
