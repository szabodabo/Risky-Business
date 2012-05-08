#!/bin/bash

cat allresults.txt | grep GLOBAL > global.txt
cat allresults.txt | grep PHASE1 > phase1.txt
cat allresults.txt | grep PHASE2 > phase2.txt

