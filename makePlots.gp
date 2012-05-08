set term postscript eps enhanced color

set style line 1 lt 1 lw 3 lc rgb "red" pt 2
set style line 2 lt 1 lw 3 lc rgb "blue" pt 2
set style line 3 lt 1 lw 3 lc rgb "green" pt 2
set style line 4 lt 2 lw 5 lc rgb "red"
set style line 5 lt 2 lw 5 lc rgb "blue"
set style line 6 lt 2 lw 5 lc rgb "green"


set xlabel "Territories (Nodes in Graph)"
set ylabel "Average Time per Round (seconds)"
#set logscale y
#set logscale x 2
set key bottom right

set output "graph.eps"
plot "global.txt" using 3:4 ls 1 title "Global" with points, \
     "phase1.txt" using 3:4 ls 2 title "Phase 1" with points, \
     "phase2.txt" using 3:4 ls 3 title "Phase 2" with points
