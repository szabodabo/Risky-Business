#!/bin/bash

for NODES in 16 32 64 128 256 512 1024 2048 4096 8192; do
	bin/gen wheel $NODES 10000 input/graphbin$NODES
done
