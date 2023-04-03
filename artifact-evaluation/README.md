# How to reporduce the experimental results of the paper
This document detailed explains all the steps to reproduce the experimental results of the paper, from the executio to the charts plotting.

## Clone the repository
```
git clone https://github.com/leonardo-panseri/cicero-on-vidor4000.git --recurse-submodules
```

## Execution on Cicero
 Before to start is well to know that the execution on the all benachmarks could take a long time. For this reason, we encorage the use of *tmux* or a similar tool to avoid execution interruption due to connection loss or accidentaly terminal disconnession.

1. Create a *tmux* session, names *test_session*:
 ```console
tmux new -s test_session
```

To detach the tmux session running *CTRL + b" then "d".

To reopen the tmux session:
 ```console
tmux attach-session -t test_session
```
2. Then, go to the folder containing the correct execution scripts
```console
cd cicero-on-vidor4000/cicero-arduino-drivers
```
3. Now, it is possible to execute all the benchmarks using the generic script:
```console
./execute_all_benchmark.sh
```

Alternatively, it is possible to execute individually each single benchmark by execution:
```console
./execute_{benchmark_name}.sh
```
**NOTE:** 
Before to execute a scripts be sure that the *arduinoport* parameter is associated to the Arduino port.

### Aggregating the execution results
Once the execution ends, each benchmark results file under the *results* folder must be aggregate:

1. ### Under the *cicero-on-vidor4000/cicero-arduino-drivers/results* is possible to find the files for each benchmark raw execution

2. ### To convert the raw file in aggregate file it is possible to exploit the

**NOTE:** the *execute_all_benchmark.sh* calls also the aggregation scripts

## Execution on ARM A53
To execute the REs benchmarks on the ARM A53 we used a Raspberry Pi 3 model B with configured the Raspberry Pi Lite OS 11.
### YARB Benchmark Suite
We exploited the **YARB** benchmark suite (https://github.com/necst/yarb) to execute the RE2 library on the used benchmarks. 

Specifically, it bases on a standard and replicabe approach to execute the tests and provides an integrate environment for REs execution retriving all the useful information.

Via YARB is possible to get the testbench, comprising the data and the REs to execute. However, to faithfully reproduce the paper experimental results, we strongly suggest to use the REs and the data we already configured. Precisely, for the tests in the paper we randomly selected a set of 200 REs from the benchmarks, and we reporte them for convenince. 

For simplicity, to test from a scratch the RE2 library on the Raspberry it possible to run:

```
cd cicero-on-vidor4000/artifact-evaluation/
./creating_re2_benchmark.sh
```
 
## Charts Plotting
Once the software and hardware executions end, it is possible to plot the data to reproduces the figures on the paper.

1. ### Creating the plotting folder structure to save the data copying the data from the source folders

Go into the *artifact-evaluation/plotting* folder
```
cd cicero-on-vidor4000/artifact-evaluation/plotting
```
Then execute the folders creator script
```
./generating_folders_structure.sh
```

3. ### Plotting the data
For execution times:
```
python charts.py 0
```
For energy efficiency:
```
python charts.py 1
```
