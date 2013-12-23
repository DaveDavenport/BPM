set term pngcairo dashed size 1200,400 
set output 'bpm.png'

set datafile separator " " 
set xdata time
set timefmt "%s"
set format x "%d/%m/%Y"
set yrange [60:180]
set xlabel "Date"
set xtic 86400 
set ylabel "BP"
set grid
set style line 1 lt 1 lc rgb "#000976" lw 2
set style line 2 lt 1 lc rgb "#007609" lw 2

plot "< ./bpm txt" using 1:2 with lines title "systolic" ls 1,\
"< ./bpm txt" using 1:3 with lines title "diastolic" ls 2
