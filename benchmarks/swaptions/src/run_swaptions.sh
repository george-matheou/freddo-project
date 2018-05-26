#!/bin/bash

# The path that the results are stored
RES=./results

#bin_path=./bin/fluidanimate

#serial_out=./results/serial_out.fluid 
#pthreads_out=./results/pthreads_out.fluid 
#tbb_out=./results/tbb_out.fluid
#freddo_out=./results/freddo_out.fluid

#input_files=(in_5K.fluid in_15K.fluid in_35K.fluid in_100K.fluid in_300K.fluid in_500K.fluid)

if [ $# -ne 5 ]; then
  echo "Usage: <rebuild [0 or 1]> <number of cores> <number of iterations> <run_serial> <only_freddo>"
  exit
fi

# Remove old files
rm $RES/*
rm out/*

only_freddo=$5
run_serial=$4
num_iterations=$3
num_cores=$2
num_kernels=`expr $num_cores - 1`

if [ $1 -eq 1 ]; then
	make -f Makefile.serial clean; make -f Makefile.serial
	#make -f Makefile.pthreads clean; make -f Makefile.pthreads
	#make -f Makefile.tbb clean; make -f Makefile.tbb
	make -f Makefile.freddo clean; make -f Makefile.freddo
fi

#num_sim=1
num_sim=102400

#input_swaps=(100 200 300 400)
input_swaps=(16 32 64)
#input_swaps=(16 100 300)
#input_swaps=(128 256 512 1024 2048 4096)

for in_swap in ${input_swaps[@]}; do
	echo "Number of Swaptions: " $in_swap
	echo "======================================="

	############################################# Execute serial implementation
	file=$RES/"serial_"$in_swap"_"$num_sim".txt"
	
if [ $run_serial -eq "1" ]; then
	i="0"
	while [ $i -lt $num_iterations ]
    do
		./swaptions_serial -nt 1 -sm $num_sim -ns $in_swap >> $file
        i=`expr $i + 1`
    done
	
	serial_time=`awk -v my_var=$num_iterations 'BEGIN{sum=0; } { if ($1 ~ /^Serial_Time/) {sum+=$2;} } END {avg_time=sum/my_var; print avg_time}' $file`
	echo "Serial Time: " $serial_time
fi
	
	
if [ $only_freddo -eq "0" ]; then
	############################################# Execute PThread Implementation
	file=$RES/"pthreads_"$in_swap"_"$num_sim"_"$num_cores".txt"
	
	i="0"
	while [ $i -lt $num_iterations ]
    do
		./swaptions_pthreads -nt $num_cores -sm $num_sim -ns $in_swap >> $file
        i=`expr $i + 1`
    done
	
	pthreads_time=`awk -v my_var=$num_iterations 'BEGIN{sum=0; } { if ($1 ~ /^PThreads_Time/) {sum+=$2;} } END {avg_time=sum/my_var; print avg_time}' $file`
	echo "PThreads Time: " $pthreads_time
	
	############################################# Execute TBB Implementation
	file=$RES/"tbb_"$in_swap"_"$num_sim"_"$num_cores".txt"
	
	i="0"
	while [ $i -lt $num_iterations ]
    do
		./swaptions_tbb -nt $num_cores -sm $num_sim -ns $in_swap  >> $file
        i=`expr $i + 1`
    done
	
	tbb_time=`awk -v my_var=$num_iterations 'BEGIN{sum=0; } { if ($1 ~ /^TBB_Time/) {sum+=$2;} } END {avg_time=sum/my_var; print avg_time}' $file`
	echo "TBB Time: " $tbb_time
fi

	# Execute FREDDO Implementation
	file=$RES/"freddo_"$in_swap"_"$num_sim"_"$num_kernels".txt"

	i="0"
	
	while [ $i -lt $num_iterations ]
    do
		./swaptions_freddo -nt $num_kernels -sm $num_sim -ns $in_swap >> $file
        i=`expr $i + 1`
    done	
	
	freddo_time=`awk -v my_var=$num_iterations 'BEGIN{sum=0; } { if ($1 ~ /^Freddo_Time/) {sum+=$2;} } END {avg_time=sum/my_var; print avg_time}' $file`
	echo "FREDDO Time: " $freddo_time

if [ $run_serial -eq "1" ]; then		
	# Print Speedups
	if [ $only_freddo -eq "0" ]; then
		echo "$serial_time $pthreads_time" | awk '{printf "-> PThreads Speedup: %.2f \n", $1/$2}'
		echo "$serial_time $tbb_time" | awk '{printf "-> TBB Speedup: %.2f \n", $1/$2}'
	fi
	
	echo "$serial_time $freddo_time" | awk '{printf "-> FREDDO Speedup: %.2f \n", $1/$2}'
	
	# Check the FREDDO Results
	# || means: if the first command succeed the second will never be executed
	cmp --silent out/freddo.out out/serial.out || echo "!!!!!!!!!!!!!!!!!!!!!!!! FREDDO Wrong Results"
fi	

	echo "======================================="
done
