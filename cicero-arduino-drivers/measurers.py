from CiceroSerial.driver import CiceroOnArduino

class regular_expression_measurer():
    """Base class for a measurer: a wrapper for some form of regex matching device that returns some kind of measures"""
    def __init__(self, name:str|list[str]):
        """Creates a new measurer, a single measure name or a list of measures names should be provided.

        :param str | list[str] name: name(s) of the measure(s)
        """
        super().__init__()
        self.name = name

    def get_name(self) -> str|list[str]:
        """Gets the name(s) of the measure(s).

        :return str|list[str]: the measure(s) name(s)
        """
        return self.name

    def execute(self, regex:str, string:bytes, O1=True, no_prefix=False, no_postfix=False, debug=False, regex_format="pythonre") -> any:
        """Measures the matching of a regex on a string.

        :param str regex: the regex to match
        :param bytes string: the bytestring to perform the matching on
        :param bool O1: if compiler optimization should be used for compiling the regex, defaults to True
        :param bool no_prefix: if the regex can match only at the start of the string, defaults to False
        :param bool no_postfix: if the regex can match only at the end of the string, defaults to False
        :param bool debug: if debug information should be printed, defaults to False
        :param str regex_format: the format of the regex, defaults to "pythonre"
        :raises NotImplementedError: if the measurer doesn't support this operation
        :return any | list[any]: a (list of) measure(s) for this execution
        """
        raise NotImplementedError()
    
    def execute_multiple_strings(self, regex:str, strings:list[bytes], O1=True, no_prefix=False, no_postfix=False, debug=False, regex_format="pythonre", skipException=True) -> list[any]:
        """Measures the matching of a regex on a list of strings.

        :param str regex: the regex to match
        :param list[bytes] strings: the bytestrings to perform the matching on
        :param bool O1: if compiler optimization should be used for compiling the regex, defaults to True
        :param bool no_prefix: if the regex can match only at the start of the strings, defaults to False
        :param bool no_postfix: if the regex can match only at the end of the strings, defaults to False
        :param bool debug: if debug information should be printed, defaults to False
        :param str regex_format: the format of the regex, defaults to "pythonre"
        :param bool skipException: if exception should be only printed, without interrupting the execution, defaults to True
        :return list[any]: a list of measures for the execution, the list can contain either single measures or lists of measures
        """
        results = []
        for string in strings:
            try:
                result = None
                result = self.execute(regex=regex, string=string, no_postfix = False, no_prefix=False, O1=True, debug=debug, regex_format=regex_format)   
            except Exception as exc:
                print('error while executing regex', regex,'\nstring [', len(string), 'chars]', string, exc)
                if not skipException:
                    raise exc
            results.append(result)
        
        return results

class CiceroOnArduino_measurer(regular_expression_measurer):
    """Measurer for CICERO on Arduino, measures are: match found (bool), number of clock cycles elapsed on the FPGA for the execution (int) and estimated execution time in microseconds on the FPGA (float)"""
    def __init__(self, arduino_port:str):
        super().__init__(["CiceroOnArduino_match[bool]", "CiceroOnArduino_exec[cc]", "CiceroOnArduino_time[micros]"])
        self.debug = False
        self.cicero = CiceroOnArduino("../cicero_compiler", arduino_port, debug=self.debug, timeout=5)
    
    def execute_multiple_strings(self, regex, strings:list, O1=True, no_prefix=True, no_postfix=True, debug=False, regex_format="pythonre", skipException=True):
        self.debug = debug
        try:
            result = self.cicero.load_regex_and_test_strings(regex, strings, regex_format=regex_format)
        except Exception as exc:
            print('error while executing regex', regex, exc)
            if not skipException:
                raise exc
            result = []
        return result

class RESULT_measurer(regular_expression_measurer):
    """Reference measurer to test if execution of other measurers is okay, the measure is: match found (bool)"""
    def __init__(self):
        super().__init__("Reference_match[bool]")

    def execute(self, regex, string, O1=True, no_prefix=True, no_postfix=True, debug=False, regex_format="pythonre"):
        import sys
        sys.path.append('../cicero_compiler')
        import golden_model
        golden_model_res = golden_model.get_golden_model_result(regex, string, no_prefix=no_prefix, no_postfix=no_postfix, frontend=regex_format)
        
        return golden_model_res