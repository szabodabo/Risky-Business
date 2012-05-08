#!/bin/bash

rm -f input/graphbin
clear
make clean
make kratos
./gengraph.sh

GRAPHS=`ls -l input | awk '{print $9; }' | grep graphbin`

for FILE in $GRAPHS; do
	rm -f graphs/graphout*
	cp input/$FILE input/graphbin
	mpirun -np 16 bin/risk --max-iterations=10
done

echo "#############################################"

#for RANK in 2 3 4 5 6 7 8 9 10 12 14 15 16; do
#	mpirun -np $RANK bin/risk --max-iterations=10
#done
