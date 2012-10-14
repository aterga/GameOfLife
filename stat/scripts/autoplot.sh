#!/bin/bash

datfiles=( `find . -maxdepth 1 -name "*.dat"` )

echo "Found .dat files for visualisation via gnuplot:"

count=0

for var in "${datfiles[@]}"
do
	echo "$var --> plot_$count.eps --> plot_$count.png"
	rm -f "plot_stat" 
	echo "set terminal postscript
              set xrange [0:]
	      set output 'plot_$count.eps'
	      plot '$var' using 1:2 with l lw 4 notitle" > plot_stat
	gnuplot plot_stat
	convert -density 300 plot_$count.eps -resize 256 plot_$count.png
	count=${count+1}
done


exit

