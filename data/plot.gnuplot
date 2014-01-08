set term pngcairo dashed size 1200,400 
set output 'bpm.png'

set fit quiet

set datafile separator " " 

set xdata time
set timefmt "%s"
set format x "%d/%m/%Y"
set yrange [60:180]
set xlabel "Date"
set xtic 86400 
set ylabel "BP"
set xtics rotate

set grid
set style line 1 lt 1 lc rgb "#000976" lw 2
set style line 2 lt 1 lc rgb "#007609" lw 2
set style line 3 lt 2 lc rgb "#760909" lw 1

FIT_LIMIT=1e-18

avgsys(x)=c+mean_systolic*(x)
fit avgsys(x) "< bpm filter txt" using 1:2 via c,mean_systolic

avgdia(x)=y0+mean_diastolic*(x)
fit avgdia(x) "< bpm filter txt" using 1:3 via y0,mean_diastolic

plot "< bpm filter txt" using 1:2 with lines title "systolic" ls 1,\
	 "< bpm filter txt" using 1:3 with lines title "diastolic" ls 2,\
	 avgsys(x) ls 3 title 'avg. systolic',\
	 avgdia(x) ls 3 title 'avg. diastolic'
