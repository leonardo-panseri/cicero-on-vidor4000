import serial
import time
from enum import Enum

def decode_bytes_as_hex(data_bytes):
    return ''.join(r'\x'+hex(byte)[2:] for byte in data_bytes)
    
def debug_print(data_bytes, bytes_num, tx):
    debug_str = ""
    debug_str += "Tx" if tx else "Rx"
    debug_str += "(" + str(bytes_num) + "): " + decode_bytes_as_hex(data_bytes)
    print(debug_str)

class CiceroOnArduino:
    CMD_REGEX = b"\x00"
    CMD_TEXT = b"\x01"

    INPUT_TERMINATOR = b"\xFF"

    MATCH_FOUND = "2"
    MATCH_NOT_FOUND = "3"
    CICERO_ERROR = "4"
    
    CICERO_CLOCK_FREQ = 24e6

    class DriverStatus(Enum):
        COMMAND_MODE = 1
        TEXT_MODE = 2
        EXECUTION_MODE = 3

    def __init__(self, cicero_compiler_path, port="COM3", baudrate=9600, timeout=1, debug=False) -> None:
        self.cicero_compiler_path = cicero_compiler_path

        self.arduino = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)
        
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
            debug_print(data_bytes, bytes_sent, True)
        
        return
    
    def _serial_read(self) -> bytes:
        read = self.arduino.read()
        
        if self.debug:
            debug_print(read, len(read), False)
            
        return read
    
    def _serial_read_until_terminator(self) -> bytes:
        read = self.arduino.read_until(self.INPUT_TERMINATOR)
        
        if self.debug:
            debug_print(read, len(read), False)
        
        # Return everything but terminator
        return read[:-1]
    
    def _send_command(self, command) -> None:
        if self.driver_status != self.DriverStatus.COMMAND_MODE:
            raise Exception("Trying to send a command while not in command mode")

        self._serial_write(command)
        
        read = self._serial_read()

        if read != bytes(command):
            raise Exception("Command not processed correctly! Expected '" + str(int.from_bytes(command, "big")) + "' but got '" + str(read, "utf-8") + "'")

    def _enter_text_mode(self) -> None:
        self._send_command(self.CMD_TEXT)
        self.driver_status = self.DriverStatus.TEXT_MODE

    def _exit_text_mode(self) -> None:
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to exit text mode while not in it")
        
        data_bytes = "-2".encode("utf-8")
        self._serial_write(data_bytes + self.INPUT_TERMINATOR)
        
        read = self._serial_read_until_terminator()
        read2 = self._serial_read()
        if read != data_bytes or read2 != self.INPUT_TERMINATOR:
            raise Exception("Could not exit text mode! Expected '-2' but got '" + str(read, "utf-8") + "'")
        
        self.driver_status = self.DriverStatus.COMMAND_MODE
    
    def _send_data_length(self, data):
        len_str = str(len(data))
        data_bytes = len_str.encode("utf-8")
        self._serial_write(data_bytes + self.INPUT_TERMINATOR)
        
        read = self._serial_read_until_terminator()
        if read != data_bytes:
            raise Exception("Command not processed correctly! Expected '" + len_str + "' but got '" + str(read, "utf-8") + "'")
    
    def _change_regex_code(self, new_regex_code) -> None:
        self._send_command(self.CMD_REGEX)
        
        self._send_data_length(new_regex_code)
        self._serial_write(new_regex_code)

        read = self._serial_read()
        if read != self.INPUT_TERMINATOR:
            raise Exception("Could not change regex code!")

        self.regex_loaded = True

    def load_regex(self, regex) -> None:
        regex_code = self._compile_regex(regex)

        if self.debug:
            print("Compiled regex code: " + decode_bytes_as_hex(regex_code))

        self._change_regex_code(regex_code)

    def load_string_and_start(self, string):
        if not self.regex_loaded:
            raise Exception("Trying to load a string before loading the regex")
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to load a string while not in text mode")
        
        self._send_data_length(string)
        self._serial_write(string, "utf-8")

        read = self._serial_read()
        if read != self.INPUT_TERMINATOR:
            raise Exception("Could not load string!")
        
        self.driver_status = self.DriverStatus.EXECUTION_MODE
        
    def wait_result(self):
        result = self._serial_read()
        if result not in [self.MATCH_FOUND.encode("utf-8"), self.MATCH_NOT_FOUND.encode("utf-8"), self.CICERO_ERROR.encode("utf-8")]:
            raise Exception("Invalid result: " + decode_bytes_as_hex(result))

        elapsedCC = self._serial_read_until_terminator()
              
        # Convert from bytestring to int
        elapsedCC = int(elapsedCC.decode("utf-8"))
        execTime = elapsedCC / self.CICERO_CLOCK_FREQ
        
        self.driver_status = self.DriverStatus.TEXT_MODE
        return result.decode("utf-8"), elapsedCC, execTime
    
    def load_regex_and_test_strings(self, regex, strings):
        results = []

        if self.debug:
            print("Loading regex: " + regex)

        self.load_regex(regex)

        self._enter_text_mode()
        for string in strings:
            if self.debug:
                print("Loading string: " + string)

            if isinstance(string, bytes):
                string = str(string, "utf-8")

            self.load_string_and_start(string)
            
            result, elapsedCC, execTime = self.wait_result()
            if result == self.CICERO_ERROR:
                print("WARN: CICERO error on regex: " + regex + ", string: " + string)
            results.append([result == self.MATCH_FOUND, elapsedCC, execTime])
        self._exit_text_mode()

        return results