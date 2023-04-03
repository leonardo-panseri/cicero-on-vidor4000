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
