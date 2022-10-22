#!/bin/bash

PY_AGGREGATOR=parser_results.py
for j in $(ls -d cicero_arduino/*)
do
	for i in $(ls ${j}/*);
do
	echo "$j"
	benchname=${j/"cicero_arduino/"/};
	echo "$benchname"
	echo "python3 ${PY_AGGREGATOR} -irf $i -gv 0 -orf $j/aggregated_${benchname}.csv -copro -freq 48;"
	echo ""
	python3 ${PY_AGGREGATOR} -irf $i -gv 0 -orf ./aggregated_${benchname}.csv -copro -freq 48 > log_${benchname}
	sleep 2
done
done
