set term postscript eps enhanced color

set style line 1 lt 1 lw 3 lc rgb "red" pt 2
set style line 2 lt 1 lw 3 lc rgb "blue" pt 2
set style line 3 lt 1 lw 3 lc rgb "green" pt 2
set style line 4 lt 2 lw 5 lc rgb "red"
set style line 5 lt 2 lw 5 lc rgb "blue"
set style line 6 lt 2 lw 5 lc rgb "green"


set xlabel "Territories (Nodes in Graph)"
set ylabel "Average Time per Round (seconds)"
set logscale y
set logscale x 2
#set key bottom right

set output "graphs.eps"
plot "out_global16.txt" using 3:4 ls 1 title "Total Execution Time" with linespoints, \
     "out_phase116.txt" using 3:4 ls 2 title "Phase 1 Execution Time" with linespoints, \
     "out_phase216.txt" using 3:4 ls 3 title "Phase 2 Execution Time" with linespoints


set xlabel "Compute Nodes (MPI Ranks)"
set ylabel "Average Time per Round (seconds)"
unset logscale
#set logscale y
#set logscale x 2
#set key bottom right
#set yrange [1:15]

set output "ranks.eps"
plot "global_nodescale.txt" using 2:4 ls 1 title "Total Execution Time" with linespoints, \
     "out_phase1_nodescale.txt" using 2:4 ls 2 title "Phase 1 Execution Time" with linespoints, \
     "out_phase2_nodescale.txt" using 2:4 ls 3 title "Phase 2 Execution Time" with linespoints
