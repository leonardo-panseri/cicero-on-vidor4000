import serial
import time
from enum import Enum

def decode_bytes_as_hex(data_bytes):
    return ''.join(r'\x'+hex(byte)[2:] for byte in data_bytes)

class CiceroOnArduino:
    CMD_REGEX = b"\x00"
    CMD_TEXT = b"\x01"

    INPUT_TERMINATOR = b"\xFF"

    MATCH_FOUND = "2"
    MATCH_NOT_FOUND = "3"

    class DriverStatus(Enum):
        COMMAND_MODE = 1
        TEXT_MODE = 2
        EXECUTION_MODE = 3

    def __init__(self, cicero_compiler_path, port="COM3", baudrate=9600, debug=False) -> None:
        self.cicero_compiler_path = cicero_compiler_path

        self.arduino = serial.Serial(port=port, baudrate=baudrate, timeout=1)
        
        self.debug = debug

        self.regex_loaded = False

        self.driver_status = self.DriverStatus.COMMAND_MODE
    
    def _compile_regex(self, regex) -> bytearray:
        import sys

        # Add Cicero compiler folder to path
        sys.path.append(self.cicero_compiler_path)

        import re2compiler
        
        code = re2compiler.compile(data=regex, O1=True, no_postfix=False, no_prefix=False)
        code_arr = code.split('\n')
        
        code_bytes = bytearray()
        for line in code_arr:
            if line.lstrip() == '':
                break
            tmp = int(line, 16)
            code_bytes += tmp.to_bytes(2, 'big')
        return code_bytes

    def _serial_write(self, data, encoding=None) -> int:
        if encoding:
            data_bytes = bytes(data, encoding)
        else:
            data_bytes = bytes(data)
            
        bytes_sent = self.arduino.write(data_bytes)
        
        if self.debug:
            print("Tx (" + str(bytes_sent) + "): " + decode_bytes_as_hex(data_bytes))
        
        return 
    
    def _send_command(self, command) -> None:
        if self.driver_status != self.DriverStatus.COMMAND_MODE:
            raise Exception("Trying to send a command while not in command mode")

        self._serial_write(command)

    def _enter_text_mode(self) -> None:
        self._send_command(self.CMD_TEXT)
        self.driver_status = self.DriverStatus.TEXT_MODE

    def _exit_text_mode(self) -> None:
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to exit text mode while not in it")
        
        self._serial_write("-2".encode("utf-8") + self.INPUT_TERMINATOR)
        self.driver_status = self.DriverStatus.COMMAND_MODE
    
    def _encode_data_length(self, data):
        return str(len(data)).encode("utf-8") + self.INPUT_TERMINATOR
    
    def _change_regex_code(self, new_regex_code) -> None:
        self._send_command(self.CMD_REGEX)
        
        self._serial_write(self._encode_data_length(new_regex_code))
        self._serial_write(new_regex_code)

        self.regex_loaded = True

    def load_regex(self, regex) -> None:
        regex_code = self._compile_regex(regex)
        self._change_regex_code(regex_code)

    def load_string_and_start(self, string):
        if not self.regex_loaded:
            raise Exception("Trying to load a string before loading the regex")
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to load a string while not in text mode")
        
        self._serial_write(self._encode_data_length(string))
        self._serial_write(string, "utf-8")
        
        self.driver_status = self.DriverStatus.EXECUTION_MODE
        
    def wait_result(self):
        result = self.arduino.read()
        
        if self.debug:
            print("Rx (1): " + decode_bytes_as_hex(result))
        
        self.driver_status = self.DriverStatus.TEXT_MODE
        return result.decode("utf-8")
    
    def load_regex_and_test_strings(self, regex, strings):
        results = []

        self.load_regex(regex)

        self._enter_text_mode()
        for string in strings:
            self.load_string_and_start(string)
            
            results += self.wait_result()
        self._exit_text_mode()

        return results


def print_results(results):
    for r in results:
        if r == CiceroOnArduino.MATCH_FOUND:
            print("OK")
        elif r == CiceroOnArduino.MATCH_NOT_FOUND:
            print("KO")
        else:
            print("ERR")


if __name__ == "__main__":
    cicero = CiceroOnArduino("../cicero_compiler")
    
    res = cicero.load_regex_and_test_strings("a+(b|c)+", ["aaab", "fdkllwk", "njfkdackljldg", "lkortioe", "jkgjdfaaabc"])
    print_results(res)
    
    print()
    
    res = cicero.load_regex_and_test_strings("g+h+", ["aaab", "fdklgghlwk", "njfkdackljldg", "lkortioeggggh", "jkgjdfaaabc"])
    print_results(res)
    
    print()
    
    res = cicero.load_regex_and_test_strings("8[a-z]+9", ["aaab", "fdkl8ggh9lwk", "njfkda8ckljldg9", "lkorti89oeggggh", "8jkgjd9faaabc"])
    print_results(res)
