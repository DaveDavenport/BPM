const char *gnuplot_file = "set term pngcairo dashed size 1200,400 \n"\
"set output 'bpm.png'\n"\
"\n"\
"set fit quiet\n"\
"\n"\
"set datafile separator " " \n"\
"\n"\
"set xdata time\n"\
"set timefmt \"%s\"\n"\
"set format x \"%d/%m/%Y\"\n"\
"set yrange [60:180]\n"\
"set xlabel \"Date\"\n"\
"set xtic 86400 \n"\
"set ylabel \"BP\"\n"\
"set xtics rotate\n"\
"\n"\
"set grid\n"\
"set style line 1 lt 1 lc rgb \"#000976\" lw 2\n"\
"set style line 2 lt 1 lc rgb \"#007609\" lw 2\n"\
"set style line 3 lt 2 lc rgb \"#760909\" lw 1\n"\
"\n"\
"FIT_LIMIT=1e-18\n"\
"";
