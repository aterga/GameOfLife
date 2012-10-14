#/bin/bash

if [ $# -lt 4 ]; then
	echo " Usage: $0  MAX_CORES  STAT_BASE  STAT_DATA_MASK  MPI_BINARY_PROG_NAME [arg1 arg2 ...]"
	exit
fi

MAX_CORES=$1
STAT_BASE=$2
FINDLINE=$3
PROGTOTEST=$4
PROGARGS=$5

# FULLPATH="$1"
# FILENAME=${FULLPATH##*/}
# BASEDIRECTORY=${FULLPATH%$FILENAME}
PROG_BIN=${PROGTOTEST##*/}
PROG_DIR=${PROGTOTEST%$PROG_BIN}
PROG_DIR=`echo $PROG_DIR | sed 's/[\/\.]//g'`
DATA_FILENAME="stat__$PROG_DIR.dat"

echo "NUM OF CORES | AVERAGE TIME"

#rm -f $DATA_FILENAME

for(( i=1; i<=$MAX_CORES; i++))
do
	for(( n=0; n<$STAT_BASE; n++))
	do
		`nice -n 100 mpirun -q -mca btl ^openib -np $i $PROGTOTEST $PROGARGS $6 $7 >> ".temp"` 
	done
	`grep "$FINDLINE" ".temp" >> ".time"` 
	`perl -pi -e 's/[:a-zA-Z\s-]*(\d+\.\d+)/$1/' ".time"`
	echo -n "           $i        " >> $DATA_FILENAME
	cat ".time"|head -10|tr " " "\t"|cut -f13|awk 'f += $1 {printf("%.6g\n",f/NR)}'|tail -1 >> $DATA_FILENAME
	rm ".temp"
	rm ".time"
done

echo >> $DATA_FILENAME

cat $DATA_FILENAME

