#!/usr/local/bin/gnuplot

set term pdf size 4.3,2.0
set size ratio 0.618
set output "round-latency.pdf"
set key outside right
set ylabel "Latency (s)"
set xlabel "Blocks per round"
set title "Node capacity 1 block/s"
set yrange [0:]

plot "one-second-per-block.dat" using ($1):2 title "All blocks" with lines lw 2 smooth unique, \
     "one-second-per-block.dat" using ($1):3 title "One block" with lines lw 2 smooth unique
