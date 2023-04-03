import sys
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats
import math
import os

execution_platforms = {}

execution_platforms['fpga-arduino']= []
execution_platforms['fpga-arduino'].append("cicero")
execution_platforms['arm-raspberry'] = []
execution_platforms['arm-raspberry'].append('re2')
avg_power = ['0.601', '2.8']


benchmarks = ['PowerEN', 'PowerEN4', 'Protomata', 'Protomata4', 'Brill', 'Brill4', 'Snort', 'Snort4']

chuck_sizes = ['1']
match_modes = ['0']
execution_engines = ['re2', 'hs']
overall_file_sizes = ['1MB']
division_factor = 1000
yaxis_steps = 5

#energy efficiency
colors_energy = ['#31a354', '#a1d99b', '#41b6c4', '#1d91c0', '#225ea8']

#avg execution time benchamrksuite
colors_exe = ['#f03b20','#feb24c','#74a9cf','#2b8cbe','#045a8d']

chart_fnt=28
leg_fnt=24
tick_fnt=21
tick_num=8
edge_line_wdth=0.5
ax_margins = 0.01
width = 0.2
x_tick_num=5

def get_avgs_from_raw_file(execution_platform, execution_engine, benchmark, chunk_size, match_mode, overall_file_size, field = 'avg_exe[ns]', sep = ' '):
    avg_final = 0
    csv_file_name = "./" + execution_platform + "/"+ execution_engine + "/" + benchmark.lower() + "/output_comp_data_" + overall_file_size + "_" + chunk_size + "_" + match_mode + ".csv"
    if os.path.exists(csv_file_name):
        explfr = pd.read_csv(csv_file_name, sep=sep)
        idx_names= explfr[explfr[field] <= 0].index
        explfr.drop(idx_names, inplace=True)
        avg_final = np.mean(explfr[field])
        #if (execution_platform == "arm") or (execution_engine == "re2" and int(chunk_size)/1000 < 16 ):
        #    avg_final = avg_final * 1000
        #    print(execution_platform + "  " +str(avg_final))
    else:
        print(csv_file_name)

    return avg_final

def plot_runtime(name, dictionary, ticks, ticks_label_text, mode):
    x = np.arange(len(ticks))  # number of grouped barplots

    dataframe = pd.DataFrame(dictionary)
    labels = dataframe.columns.values
    maxres = max(dataframe.max())
    normalized_df = dataframe
    print(normalized_df)

    fig, ax = plt.subplots(1, 1, figsize=(16,10))
    ax.tick_params(axis='both', which='both', labelsize=tick_fnt)
    x_labels = ticks 
    x_ticks = np.arange(len(ticks))

    offs = np.arange((-len(labels)/2)*width, (len(labels)/2)*width, width) + 0.1
    i=0
    colors = colors_exe
    if mode == 1:
        colors= colors_energy
        
    for l in labels:
        ax.bar(x_ticks+offs[i], normalized_df[l], width=width, color=colors[i], edgecolor='black',linewidth=edge_line_wdth, zorder=3)
        i=i+1
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels)
    ax.margins(x=ax_margins)

    ymax = np.max(ax.get_ylim())
    ymax_ndigit = len(str(ymax))
    ymax = int(ymax / ymax_ndigit) * ymax_ndigit
    ymax = int(ymax + ymax * 1/3)
    
    ymax = ymax + (yaxis_steps - ymax % yaxis_steps)

    ax.set_yticks(np.linspace(0, ymax ,num=yaxis_steps))

    if mode == 1:
        ax.set_ylabel('Avg. Energy Efficiency Log Scale [bench/J]', fontsize=chart_fnt)
    else:
        ax.set_ylabel('Avg. Execution Time Log Scale [\u03BCs]', fontsize=chart_fnt)
    ax.set_xlabel(ticks_label_text, fontsize=chart_fnt)
    ax.grid(visible=True, which='major', linestyle='dotted',axis='y',zorder=0)
    ax.legend(labels, loc='upper center', bbox_to_anchor=(0.5, 1.11) ,ncol=int(len(labels)),fontsize=leg_fnt)
    yticks = ax.yaxis.get_major_ticks()
    ax.set_yscale('log')
    fig.savefig(name + '.svg', format='svg', bbox_inches = 'tight', dpi=1200)
    fig.savefig(name + '.pdf', format='pdf', bbox_inches = 'tight', dpi=1200)
    plt.close(fig)


def main():
    mode = int(sys.argv[1])
    all_execution_engines = []

    dictionary = {}
    for execution_platform in execution_platforms:
        for execution_engine in execution_platforms[execution_platform]:
            dictionary[str(execution_platform) + '_' + str(execution_engine)] = []    
            for overall_file_size in overall_file_sizes:          
                for benchmark in benchmarks:
                    chunks_avg = []
                    for chuck_size in chuck_sizes:
                        match_avg = 0
                        for match_mode in match_modes:
                            if execution_engine == 'cicero':
                                match_avg = get_avgs_from_raw_file(execution_platform, execution_engine, benchmark, str((int(chuck_size) * 1000)), match_mode, overall_file_size, 'AVG_Time[ns]', ',')/division_factor 
                            else:
                                match_avg = get_avgs_from_raw_file(execution_platform, execution_engine, benchmark, str((int(chuck_size) * 1000)), match_mode, overall_file_size, 'avg_exe[ns]', ' ')/division_factor
                        chunks_avg.append(match_avg)
                        #energy efficinecy code
                    avg_power_index = 1
                    if execution_engine == 'cicero':
                        avg_power_index = 0
                    if mode == 1:
                        dictionary[str(execution_platform) + '_' + str(execution_engine)].append((1/np.mean(match_avg))/float(avg_power[avg_power_index]))
                    else:
                        dictionary[str(execution_platform) + '_' + str(execution_engine)].append(np.mean(chunks_avg))
            if 0 in dictionary[str(execution_platform) + '_' + str(execution_engine)]:
                dictionary.pop(str(execution_platform) + '_' + str(execution_engine))
    print(dictionary)
    if any(dictionary):
        name = "exec_time"
        if mode == 1:
            name = "energy_eff"
        name="./" + name + "_benchmarks_per_chunks_" + execution_platform + '_' + execution_engine + "_" + overall_file_size + "_" + match_mode
        plot_runtime(name, dictionary,  benchmarks, 'Benchmark', mode)

if __name__ == '__main__':
    main()