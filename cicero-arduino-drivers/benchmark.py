from typing import Iterable
from measurers import regular_expression_measurer, RESULT_measurer, CiceroOnArduino_measurer

import csv
import argparse
import os.path
from itertools import chain, product
from tqdm import tqdm
import numpy as np

RESULTS_DIRECTORY = "results"
INPUTS_DIRECTORY = "inputs"

def chunks(iterable: Iterable, chunk_size: int):
    """Yield successive n-sized chunks from an iterable.

    :param Iterable iterable: the iterable to split into chunks
    :param int chunk_size: the size of the chunks
    :yield any: the next chunk from the iterable
    """
    for i in range(0, len(iterable), chunk_size):
        yield iterable[i:i + chunk_size]

def load_regexes(benchmark_name:str, start_index:int, end_index:int) -> list[str]:
    """Loads a list of regexes from a file.

    :param str benchmark_name: the name of the benchmark for which to load regexes
    :param int start_index: the start index of the subset of regexes to return
    :param int end_index: the end index of the subset of regexes to return
    :return list[str]: the list of regexes
    """
    regexes = []
    
    regex_file = INPUTS_DIRECTORY + "/" + benchmark_name + "/regex.txt"
    # Read regexes from file
    with open(regex_file, 'r') as f:
        regexes = f.readlines()[start_index:end_index]

        # Eliminate end of line 
        regexes = list(map(lambda x: x[:-1], regexes))

    return regexes

def load_strings(benchmark_name:str, reduced_input:bool, start_index:int, end_index:int, max_length:int) -> list[bytes]:
    """Loads a list of strings from file.

    :param str benchmark_name: the name of the benchmark for which to load strings
    :param bool reduced_input: if strings should be loaded from the reduced version of the input
    :param int start_index: the start index of the subset of strings to return
    :param int end_index: the end index of the subset of strings to return
    :param int max_length: the max length of the strings, if there are longer strings they will be split into chunks and all chunks will be returned
    :return list[bytes]: the list of bytestrings representing the strings
    """
    strings = []
    
    string_file = INPUTS_DIRECTORY + "/" + benchmark_name + "/"
    string_file += "reduced." if reduced_input else ""
    string_file += "input.txt"
    # Read strings from file as bytestrings
    with open(string_file, 'rb') as f:
        strings = f.read().split(b'\n')

        # Eliminate end of line and split every string in chunks of max lenght 'max_length'
        strings = list(chain.from_iterable(map(lambda x: chunks(x[:-1], max_length), strings)))[start_index:end_index]

    return strings

def execute_benchmark(measurer_list:list[regular_expression_measurer], progress_bar:tqdm, regexes:list[str], strings:list[bytes], regex_format:str, skip_exceptions:bool, debug:bool) -> dict[tuple,bool|str|int|float]:
    """Executes a benchmark.

    :param list[regular_expression_measurer] measurer_list: list of measurers to execute the benchmark with
    :param tqdm progress_bar: progress bar to update at each iteration
    :param list[str] regexes: the list of regexes to test
    :param list[bytes] strings: the list of strings to test
    :param str regex_format: the format of given regexes
    :param bool skip_exceptions: if exceptions should not stop the computation, being only printed instead
    :param bool debug: if febug messages should be shown
    :return dict[tuple,bool|str|int|float]: results will be in the form: (regex, string, measure_name): measure_value
    """
    # This dict will contain results in the form:
    #   (regex, string, measure_name): measure_value
    results = {}

    # Foreach tuple in the cartesian product of regexes and measurers
    for regex, measurer in product(regexes, measurer_list) :
        try:
            results_per_regex = measurer.execute_multiple_strings(regex=regex, strings=strings, O1=True, no_postfix=False, no_prefix=False, regex_format=regex_format, debug=debug, skipException=skip_exceptions)   
        except KeyboardInterrupt:
            print ("[CTRL+C detected]")
            # Skip to save file
            break
        except Exception as exc:
            print('error while executing regex', regex,'\n',  exc)
            if not skip_exceptions:
                raise exc
            
            # Put null values in the result to show that something went wrong
            if isinstance(measurer.get_name(), list):
                results_per_regex =  [[None for _ in strings] for _ in measurer.get_name()]
            else:
                results_per_regex =  [None for _ in strings]
        
        # len(strings) computation have been done, update the progress bar
        progress_bar.update(len(strings))

        # If the measurer returns multiple measures, we need to correctly put all of them in the results
        if isinstance(measurer.get_name(), list):
            for string, result in zip(strings, results_per_regex):
                for measure_name, measure_value in zip(measurer.get_name(), result):
                    results[(regex, string, measure_name)] = measure_value
        # If the measurer returns only one measure, put it directly in the results
        else:
            measure_name = measurer.get_name()
            for string, result in zip(strings,results_per_regex):
                    results[(regex,string,measure_name)]= result

    return results

def save_results_to_file(results:dict[tuple,bool|str|int|float], benchmark_name:str) -> None:
    """Saves the results to a CSV file. They will be organized in sections, one for each string,
    and each section will have a list of all regexes with all the associated measures for that computation.

    :param dict[tuple,bool | str | int | float] results: the results to save
    :param str benchmark_name: the name of the benchmark that will be appended to the name of the file
    """
    # Build an index of the results with this structure:
    #   string1:
    #     regex1: [measure1, measure2, ...]
    #     regex2: [measure1, measure2, ...]
    #     ...
    #   ...
    result_index = {}
    for regex, string, measurer in results:
        if not(string in result_index):
            result_index[string]={}

        if not(regex in result_index[string]):
            result_index[string][regex]=[]

        result_index[string][regex].append(measurer)

    if not os.path.isdir(RESULTS_DIRECTORY):
        os.mkdir(RESULTS_DIRECTORY)

    # Write results to CSV file
    with open(f'{RESULTS_DIRECTORY}/measure_{benchmark_name}.csv', 'w', newline='') as csvfile:
        fout = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_MINIMAL)

        # Write a section for each string
        for string in result_index:
            # First, write the string
            fout.writerow(['string: ', string, '', ''])

            # Then write the headers of the columns of this section
            # The headers will be all the measure names
            names=[]
            for measurer in measurer_list:
                if isinstance(measurer.get_name(), list):
                    names += [*measurer.get_name()]
                else:
                    names.append(measurer.get_name())
            fout.writerow(['regex', *names])

            # Then write the regexes and their associated results
            for regex in result_index[string]:
                result_line = [results[(regex,string,e)] if (regex,string,e) in results else None for e in names]
                fout.writerow([regex,  *result_line ])

def get_sampled_indexes(original_array: list, size: int) -> list[int]:
    """Gets a list of randomly chosen indexes of the array.

    :param list original_array: the array to generate indexes for
    :param int size: the number of indexes to randomly choose
    :return list[int]: the list containing the randomly chosen indexes of the array
    """
    samples = np.random.normal(size=size)
    normalized_samples = (samples - min(samples)) / (max(samples) - min(samples))
    return (normalized_samples * (len(original_array) - 1)).astype(np.int32)

def generate_random_sample(benchmark_name:str, regexes:list[str], strings:list[bytes], size:int) -> None:
    """Genereates two lists of randomly chosen indexes for the regexes and strings, then save them to file and exit.

    :param str benchmark_name: the name of the benchmark, will be used in the name of the file
    :param list[str] regexes: the list of regexes
    :param list[bytes] strings: the list of strings
    :param int size: the number of indexes to randomly choose
    """
    regexes_sampled_indexes = get_sampled_indexes(regexes, size)
    strings_sampled_indexes = get_sampled_indexes(strings, size)

    np.save(INPUTS_DIRECTORY + "/" + benchmark_name + "/rand.input.index", strings_sampled_indexes)
    np.save(INPUTS_DIRECTORY + "/" + benchmark_name + "/rand.regex.index", regexes_sampled_indexes)

    print("Done! Now execute with '-loadsample' argument.")

    exit()


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description='test regular expression matching')
    arg_parser.add_argument('-benchmark',         type=str,  help='name of the benchmark in execution',                                                default='protomata')
    arg_parser.add_argument('-format',            type=str,  help='regex input format',                                                                default='pcre')
    arg_parser.add_argument('-startreg',          type=int,  help='index first reg.to restrict num of regexp',                                         default=0)
    arg_parser.add_argument('-endreg',            type=int,  help='index end reg.to restrict num of regexp',                                           default=None)
    arg_parser.add_argument('-reducedstr',                   help='load strings from the reduced file',                          action='store_true',  default=False)
    arg_parser.add_argument('-maxstrlen',         type=int,  help='max length of string. to restrict string size',                                     default=1024)
    arg_parser.add_argument('-startstr',          type=int,  help='index first str. to restrict num of strings',                                       default=0)
    arg_parser.add_argument('-endstr',            type=int,  help='index end string. to restrict num of strings',                                      default=None)
    arg_parser.add_argument('-debug',                        help='execute in debug mode',                                       action='store_true',  default=False)
    arg_parser.add_argument('-skipException',                help='skip exceptions',                                             action='store_true',  default=False)
    arg_parser.add_argument('-randomsample',      type=int,  help='generate a new random sample of the given size',                                    default=None)
    arg_parser.add_argument('-loadregexsample',              help='execute with the previously generated random regex sample',   action="store_true",  default=False)
    arg_parser.add_argument('-loadstringsample',             help='execute with the previously generated random string sample',  action="store_true",  default=False)
    arg_parser.add_argument('-arduinoport',       type=str,  help='name of serial port where the Arduino running CICERO is',                           default='COM3')

    args = arg_parser.parse_args()

    # Any different method of regex matching is measured through an instance of a regular_expression_measurer subclass,
    # which expose method 'execute_multiple_strings()' that returns either one or a list of results.  
    measurer_list = [RESULT_measurer(), CiceroOnArduino_measurer(args.arduinoport)]
    
    # Check if the specified benchmark exists in the input folder
    if not os.path.isdir(INPUTS_DIRECTORY + "/" + args.benchmark):
        print(f"Benchmark '{args.benchmark}' does not exist!")
        exit()
    
    # Load regexes and strings from file
    regexes = load_regexes(args.benchmark, args.startreg, args.endreg)
    strings = load_strings(args.benchmark, args.reducedstr, args.startstr, args.endstr, args.maxstrlen)

    # If the sample size is provided, generate two new array of randomly chosen indexes for the regexes and strings list respectively, and save them to file
    if args.randomsample:
        generate_random_sample(args.benchmark, regexes, strings, args.randomsample)
    
    # If the load sample argument is provided, keep in the regexes and/or strings list only the indexes loaded from file
    if args.loadregexsample:
        regexes_sampled_indexes = np.load(INPUTS_DIRECTORY + "/" + args.benchmark + "/rand.regex.index.npy")
        regexes = [regexes[i] for i in regexes_sampled_indexes]

    if args.loadstringsample:
        strings_sampled_indexes = np.load(INPUTS_DIRECTORY + "/" + args.benchmark + "/rand.input.index.npy")
        strings = [strings[i] for i in strings_sampled_indexes]

    # Calculate the total number of executions to initialize the progress bar
    total_number_of_executions = len(strings)*len(regexes)*len(measurer_list)
    progress_bar = tqdm(total=total_number_of_executions)

    # Execute the benchmark with the given measurers
    results = execute_benchmark(measurer_list, progress_bar, regexes, strings, args.format, args.skipException, args.debug)

    # Finally, save the results to a CSV file args.loadstringsample
    file_name = f"{args.benchmark}_"
    file_name += "rand_" if args.loadregexsample else f"{args.startreg}-{args.endreg}_"
    file_name += "rand" if args.loadstringsample else f"{args.startstr}-{args.endstr}"
    save_results_to_file(results, file_name)
