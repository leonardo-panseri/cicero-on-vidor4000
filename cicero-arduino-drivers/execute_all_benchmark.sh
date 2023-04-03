#!/bin/bash
arduinoport=/dev/ttyACM0
declare -a BenchmarksArray=("poweren" "poweren4" "brill" "brill4" "snort" "snort4" "protomata" "protomata4")
while getopts p:a: flag
do
    case "${flag}" in
        p) arduinoport=${OPTARG};;
    esac
done
echo "ARDUINO PORT: $arduinoport"

for val in ${BenchmarksArray[@]}; do
    echo "BENCHMARK: $val"
    python3.10 benchmark.py -benchmark=$val -format=pcre -loadregexsample -reducedstr -skipException -arduinoport=$arduinoport
    #put here the data aggregation script 
done