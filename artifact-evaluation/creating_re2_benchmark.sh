#!/bin/bash
git clone https://github.com/necst/yarb.git
cd yarb
#!/bin/bash
benchmark=("Brill" "PowerEN" "Protomata" "Snort")

platform=("re2")
chunk_size=("1")
data_file=("data_1MB")
match_mode=("0")
start_regex=0
end_regex=4

#additional setup for GPU test
#can be removed for other platforms test
for p in "${platform[@]}"
  do
    make ${p}_compile
  done

for b in "${benchmark[@]}"
do
  benchmark_lowercase=$(echo "$b" | awk '{print tolower($0)}')
  echo ${benchmark_lowercase}
  for mode in {0..1}
  do
    echo $mode
    benchmark_to_exe=${b}
    if [ $mode -eq "0" ]; then
      make get_${benchmark_lowercase}_tb
    else
      cp ${b} ${b}4 -r
      benchmark_to_exe=${b}4
      benchmark_lowercase="$benchmark_lowercase"4
    fi
    
    cp ../../cicero-arduino-drivers/inputs/random_regex/${benchmark_lowercase}_regex.txt ${benchmark_to_exe}/regex.txt
    for p in "${platform[@]}"
    do
      for cs in "${chunk_size[@]}"
      do
        for df in "${data_file[@]}"
        do
            for mm in "${match_mode[@]}"
            do
              make ${p}_build TESTBENCH_NAME=${benchmark_to_exe} MATCH_MODE=${mm} DATA_FILE=${df} CHUNCK_SIZE_VAL=${cs} GPU_WORK=${gpu_work} GPU_WORK_ALGORITHM=${gpu_work_algorithm}
              make ${p}_test TESTBENCH_NAME=${b} MATCH_MODE=${mm} DATA_FILE=${df} CHUNCK_SIZE_VAL=${cs} GPU_WORK=${gpu_work} GPU_WORK_ALGORITHM=${gpu_work_algorithm}
            done
        done
      done
    done
  done
done