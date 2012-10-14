#/bin/bash

if [ $# -ne 8 ]; then
	echo " Usage: $0  MAX_CORES  STAT_BASE  STAT_DATA_MASK  MPI_BIN_PROG_NAME  FIELD_X_SIZE  FIELD_Y_SIZE  MAX_GENERATIONS  GEN_STEP"
	exit
fi

for(( i=1; i<=$7; i+=$8))
do
	./stat.sh $1 $2 $3 $4 $5 $6 $i
done

