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
    python parser_results.py -irf ./results/measure_${val}_rand_0-None.csv -gv 1 -orf ./results_aggr/${val}_output_comp_data_1MB_1000_0.csv -copro -freq 48
done