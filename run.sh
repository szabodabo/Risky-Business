#!/bin/bash

rm -f input/graphbin

GRAPHS=`ls -l input | awk '{print $9; }' | grep graphbin`

for FILE in $GRAPHS; do
	cp input/$FILE input/graphbin
	mpirun -np 16 bin/risk --max-iterations=10
done
