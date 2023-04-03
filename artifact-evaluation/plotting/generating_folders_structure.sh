#!/bin/bash
declare -a BenchmarksArray=("poweren" "poweren4" "brill" "brill4" "snort" "snort4" "protomata" "protomata4")
declare -a DataFolders=("arm-raspberry/re2" "fpga-arduino/cicero")

mkdir fpga-arduino/cicero -p

for i in "${DataFolders[@]}"
do
    mkdir $i -p
    for x in "${BenchmarksArray[@]}"
    do
        mkdir $i/$x -p  
    done
done

for x in "${BenchmarksArray[@]}"
do
    cp ../../cicero-arduino-drivers/results_aggr/${x}_output_comp_data_1MB_1000_0.csv fpga-arduino/cicero/$x/output_comp_data_1MB_1000_0.csv
    benchmark=$(echo "${x:0:1}" | awk '{print toupper($0)}')${x:1}
    cp ../yarb/${benchmark}_massive_test/re2/output_comp_data_1MB_1000_0.csv arm-raspberry/re2/$x/output_comp_data_1MB_1000_0.csv
done