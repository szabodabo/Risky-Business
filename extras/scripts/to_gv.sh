#!/bin/bash

CONVERTER_BIN=bin/to_graphviz
GRAPH_BIN_DIR=graphs
GRAPHVIZ_OUT_DIR=graphs/graphviz
PNG_OUT_DIR=graphs/graphviz/png

GRAPH_FILES=`ls -l $GRAPH_BIN_DIR | grep graphout | awk '{ print $9; }'`

for GRAPH in $GRAPH_FILES; do
	$CONVERTER_BIN $GRAPH_BIN_DIR/$GRAPH > $GRAPHVIZ_OUT_DIR/$GRAPH.gv
	neato $GRAPHVIZ_OUT_DIR/$GRAPH.gv -Tpng -o $PNG_OUT_DIR/$GRAPH.png

done
