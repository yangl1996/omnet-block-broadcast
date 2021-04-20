#!/usr/local/bin/gnuplot

set term pdf size 4.3,2.0
set size ratio 0.618
set output "round-latency.pdf"
set key outside right
set ylabel "Latency (s)"
set xlabel "Blocks per round"
set notitle
set yrange [0:]

plot "one-second-per-block.dat" using ($1):2 title "1 Block/s" with lines lw 2 smooth unique
