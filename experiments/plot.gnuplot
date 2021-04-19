#!/usr/local/bin/gnuplot

set term pdf size 4.3,2.0
set size ratio 0.618
set output "round-latency.pdf"
set key outside right
set ylabel "Latency (s)"
set xlabel "Tx Throughput / Capacity"
set notitle
set xrange [0:1]

plot "big-blocks.dat" using ($1*1000*10/10):2 title "Big block (10x)" with lines lw 2, \
     "tiny-blocks.dat" using ($1*1000/10/10):2 title "Tiny block (0.1x)" with lines lw 2, \
     "small-blocks.dat" using ($1*1000/10):2 title "Small block (1x)" with lines lw 2
