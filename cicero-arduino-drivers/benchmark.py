from CiceroSerial.driver import CiceroOnArduino

import csv
import os 
import argparse
import sys
from itertools import chain, product
from tqdm import tqdm
import numpy as np


class regular_expression_measurer():
    def __init__(self, name):
        super().__init__()
        self.name = name

    def get_name(self):
        return self.name

    def execute(self, regex, string, O1=True, no_prefix=True, no_postfix=True, debug=False):
        raise NotImplementedError()
    
    def execute_multiple_strings(self, regex, strings:list, O1=True, no_prefix=True, no_postfix=True, debug=False, skipException=True):
        results = []
        for string in strings:
            try:
                result = None
                result = self.execute(regex=regex, string=string, no_postfix = False, no_prefix=False, O1=True, debug=debug )   
            except Exception as exc:
                print('error while executing regex', regex,'\nstring [', len(string), 'chars]', string, exc)
                if not skipException:
                    raise exc
            results.append(result)
        
        return results

class CiceroOnArduino_measurer(regular_expression_measurer):
    def __init__(self, frontend='pythonre'):
        super().__init__(["CiceroOnArduino_match[bool]", "CiceroOnArduino_exec[cc]", "CiceroOnArduino_time[s]"])
        self.frontend = frontend
        self.debug = False
        self.cicero = CiceroOnArduino("../cicero_compiler", debug=self.debug, timeout=5)
    
    def execute_multiple_strings(self, regex, strings:list, O1=True, no_prefix=True, no_postfix=True, debug=False, skipException=True):
        self.debug = debug
        try:
            result = self.cicero.load_regex_and_test_strings(regex, strings)
        except Exception as exc:
            print('error while executing regex', regex, exc)
            if not skipException:
                raise exc
            result = []
        return result

class RESULT_measurer(regular_expression_measurer):
    def __init__(self):
        super().__init__("RESULT")

    def execute(self, regex, string, O1=True, no_prefix=True, no_postfix=True, debug=False):
        import sys
        sys.path.append('../cicero_compiler')
        import golden_model
        golden_model_res = golden_model.get_golden_model_result(regex, string, no_prefix=no_prefix, no_postfix=no_postfix,frontend=args.format)
        
        return golden_model_res

def chunks(lst, n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        yield lst[i:i + n]

arg_parser = argparse.ArgumentParser(description='test regular expression matching')
arg_parser.add_argument('-maxstrlen'		, type=int , help='max length of string. to restrict string size'	                                                                	   , default=1024)
arg_parser.add_argument('-startstr'		    , type=int , help='index first str. to restrict num of strings'	                                                                		   , default=0   )
arg_parser.add_argument('-endstr'		    , type=int , help='index end string. to restrict num of strings'	                                                            		   , default=None)
arg_parser.add_argument('-startreg'		    , type=int , help='index first reg.to restrict num of regexp'	                                                                		   , default=0   )
arg_parser.add_argument('-endreg'		    , type=int , help='index end reg.to restrict num of regexp'		                                                                		   , default=None)
arg_parser.add_argument('-strfile'		    , type=str , help='file containing strings'  	                                        					    						   , default='protomata.input.txt')
arg_parser.add_argument('-regfile'		    , type=str , help='file containing regular expressions'    	                            					    						   , default='protomata.regex.txt')
arg_parser.add_argument('-debug'	                   , help='execute in debug mode'                                    									,action='store_true'       , default=False)
arg_parser.add_argument('-skipException'	           , help='skip exceptions'                                    									        ,action='store_true'       , default=False)
arg_parser.add_argument('-format'	        , type=str , help='regex input format'                                    									                               , default='pythonre')
arg_parser.add_argument('-benchmark'        , type=str , help='name of the benchmark in execution'																					   , default='protomata')

args = arg_parser.parse_args()

#any program and coprocessors have a specific method
# to measure the time taken to complete a match.
# These methods are encapsulated into an instance of measurer subclass,
# which expose a method(execute()) to measure time required to match.  
# measurer_list is filled by measurer depending on user requests.
measurer_list = [RESULT_measurer(), CiceroOnArduino_measurer()]

str_lines   = []
#read string file
with open(args.strfile, 'rb') as f:
    str_lines = f.read().split(b'\n')[args.startstr:args.endstr]
    #eliminate end of line and split them in chunks.
    str_lines = list(chain.from_iterable(map(lambda x: chunks(x[:-1],args.maxstrlen),str_lines)))
regex_lines = []
#open regex file 
with open(args.regfile, 'r') as f:
    regex_lines = f.readlines()[args.startreg:args.endreg]
    #eliminate end of line 
    regex_lines = list(map(lambda x: x[:-1], regex_lines))

total_number_of_executions = len(str_lines)*len(regex_lines)*len(measurer_list)
progress_bar = tqdm(total=total_number_of_executions)

results = {}

#foreach regex, executor
for r,e in product(regex_lines, measurer_list) :
    try:
        results_per_regex = e.execute_multiple_strings(regex=r, strings=str_lines, no_postfix = False, no_prefix=False, O1=True, debug=args.debug, skipException=args.skipException )   
    except KeyboardInterrupt:
        print ("[CTRL+C detected]")
        #skip to save file.
        break
    except Exception as exc:
        print('error while executing regex', r,'\n',  exc)
        if not args.skipException:
            raise exc
        if isinstance(e.get_name(), list):
            results_per_regex =  [[None for _ in str_lines] for _ in e.get_name()]
        else:
            results_per_regex =  [None for _ in str_lines]
    
    progress_bar.update(len(str_lines))

    if isinstance(e.get_name(), list):
        for l,result in zip(str_lines,results_per_regex):
            for e_name, result_per_e in zip(e.get_name(),result):
                results[(r,l,e_name)]= result_per_e
    else:
        e_name = e.get_name()
        for l,result in zip(str_lines,results_per_regex):
                results[(r,l,e_name)]= result

result_index = {}
for r,l,e in results:
    if not(l in result_index):
        result_index[l]={}

    if not(r in result_index[l]):
        result_index[l][r]=[]

    result_index[l][r].append(e)

#open log file and log results.
with open(f'measure_{args.benchmark}_{args.startreg}-{args.endreg}.csv', 'w', newline='') as csvfile:
    fout = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_MINIMAL)

    for l in result_index:
        fout.writerow(['string: ', l, '', ''])
        names=[]
        for e in measurer_list:
            if isinstance(e.get_name(), list):
                names += [*e.get_name()]
            else:
                names.append(e.get_name())
        fout.writerow(['regex', *names])

        for r in result_index[l]:
            result_line = [results[(r,l,e)] if (r,l,e) in results else None for e in names]
            fout.writerow([r,  *result_line ])