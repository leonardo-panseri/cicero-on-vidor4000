import serial
import sys
from enum import Enum

def decode_bytes_as_hex(data_bytes: bytes) -> str:
    """Formats a bytestring as a string containing the hex representation of the bytes.

    :param bytes data_bytes: the bytes to be formatted
    :return str: the formatted string
    """
    return ''.join(r'\x'+hex(byte)[2:] for byte in data_bytes)
    
def debug_print(data_bytes: bytes, bytes_num: int, tx: bool) -> None:
    """Prints the bytes sent or received.

    :param bytes data_bytes: the bytes sent or received
    :param int bytes_num: the number of bytes sent or received
    :param bool tx: True if the bytes were sent, False if they were received
    """
    debug_str = ""
    debug_str += "Tx" if tx else "Rx"
    debug_str += "(" + str(bytes_num) + "): " + decode_bytes_as_hex(data_bytes)
    print(debug_str)

class CiceroOnArduino:
    """Driver for CICERO on the Arduino MKR Vidor 4000."""
    # Commands for the Arduino
    CMD_REGEX = b"\x00"
    CMD_TEXT = b"\x01"
    CMD_EXIT_TEXT = b"-2\xFF"

    # Special character to be used as a delimiter
    INPUT_TERMINATOR = b"\xFF"

    # Possible values returned from the Arduino that represent the result of the computation
    MATCH_FOUND = "2"
    MATCH_NOT_FOUND = "3"
    CICERO_ERROR = "4"
    
    # CICERO clock frequency on the FPGA on the Arduino to estimate execution time
    CICERO_CLOCK_FREQ = 24e6

    class DriverStatus(Enum):
        """Enum containing the possible statuses for the driver."""
        COMMAND_MODE = 1
        TEXT_MODE = 2
        EXECUTION_MODE = 3

    def __init__(self, cicero_compiler_path:str, port:str, baudrate=9600, timeout=1, debug=False) -> None:
        """Creates a new instance of the driver.

        :param str cicero_compiler_path: the path (relative or absolute) where to find CICERO compiler
        :param str port: the port where to find the Arduno serial connection
        :param int baudrate: the baudrate for the Arduino serial connection, defaults to 9600
        :param int timeout: the time to wait for data sent from Arduino (in seconds), defaults to 1
        :param bool debug: if debug messages should be printed, defaults to False
        """
        self.cicero_compiler_path = cicero_compiler_path
        # Add CICERO compiler folder to PATH
        sys.path.append(self.cicero_compiler_path)

        self.arduino = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)
        
        self.debug = debug
        self.regex_loaded = False
        # On power up or reset the Arduino is in command mode
        self.driver_status = self.DriverStatus.COMMAND_MODE
    
    def _compile_regex(self, regex: str, regex_format = "pythonre") -> bytearray:
        """Compiles the regex to obtain bytecode for CICERO.

        :param str regex: the regex to compile
        :param str regex_format: the format of the regex (see compiler), defaults to "pythonre"
        :return bytearray: the compiled bytecode
        """
        # Import CICERO compiler (folder containing it has been added to PATH)
        import re2compiler
        
        code = re2compiler.compile(data=regex, O1=True, no_postfix=False, no_prefix=False, frontend=regex_format)

        # Code is returned as a string containing bytes represented as hex separated by '\n'
        # We need to convert it into a byte array that can be sent to Arduino
        code_arr = code.split('\n')
        
        code_bytes = bytearray()
        for line in code_arr:
            if line.lstrip() == '':
                break
            tmp = int(line, 16)
            code_bytes += tmp.to_bytes(2, 'big')
        return code_bytes

    def _serial_write(self, data: bytes|str, encoding:str=None) -> int:
        """Writes data to Arduino through serial.

        :param bytes | str data: the data to be sent
        :param str encoding: if 'data' is a string, the encoding to use to convert it to bytes, defaults to None
        :return int: the number of bytes written
        """
        if encoding:
            data_bytes = bytes(data, encoding)
        else:
            data_bytes = bytes(data)
            
        bytes_sent = self.arduino.write(data_bytes)
        
        if self.debug:
            debug_print(data_bytes, bytes_sent, True)
        
        return
    
    def _serial_read(self) -> bytes:
        """Reads one byte from Arduino through serial.

        :return bytes: the byte read
        """
        read = self.arduino.read()
        
        if self.debug:
            debug_print(read, len(read), False)
            
        return read
    
    def _serial_read_until_terminator(self) -> bytes:
        """Reads bytes from Arduino through serial until INPUT_TERMINATOR is found.

        :return bytes: all the bytes read, but INPUT_TERMINATOR
        """
        read = self.arduino.read_until(self.INPUT_TERMINATOR)
        
        if self.debug:
            debug_print(read, len(read), False)
        
        # Return everything but terminator
        return read[:-1]
    
    def _send_command(self, command: bytes) -> None:
        """Sends a command to Arduino.

        :param bytes command: the command to send, should be one of the CMD_* variables
        :raises Exception: if the driver is not in command mode or if Arduino doesn't respond
        """
        if self.driver_status != self.DriverStatus.COMMAND_MODE:
            raise Exception("Trying to send a command while not in command mode")

        self._serial_write(command)
        
        read = self._serial_read()

        if read != bytes(command):
            raise Exception("Command not processed correctly! Expected '" + str(int.from_bytes(command, "big")) + "' but got '" + str(read, "utf-8") + "'")

    def _enter_text_mode(self) -> None:
        """Enters the driver status where strings can be sent to be analyzed by CICERO."""
        self._send_command(self.CMD_TEXT)
        self.driver_status = self.DriverStatus.TEXT_MODE

    def _exit_text_mode(self) -> None:
        """Exits the driver status where strings can be sent to be analyzed by CICERO and returns to command mode.

        :raises Exception: if the driver is not in text mode or if Arduino doesn't respond
        """
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to exit text mode while not in it")
        
        # We can't use _send_command() as it will check if we are in command mode
        self._serial_write(self.CMD_EXIT_TEXT)
        
        # First read is to check if the command has been received correctly
        read = self._serial_read_until_terminator()
        # Second read is to check if Arduino actually exited text mode
        read2 = self._serial_read()
        if read != self.CMD_EXIT_TEXT[:-1] or read2 != self.INPUT_TERMINATOR:
            raise Exception("Could not exit text mode! Expected '-2' but got '" + str(read, "utf-8") + "'")
        
        self.driver_status = self.DriverStatus.COMMAND_MODE
    
    def _send_data_length(self, data: bytes|str) -> None:
        """Sends to Arduino through serial the length of the data that will be sent afterward.

        :param bytes | str data: the data that will be sent
        :raises Exception: if Arduino doesn't respond
        """
        len_str = str(len(data))
        data_bytes = len_str.encode("utf-8")
        self._serial_write(data_bytes + self.INPUT_TERMINATOR)
        
        read = self._serial_read_until_terminator()
        if read != data_bytes:
            raise Exception("Command not processed correctly! Expected '" + len_str + "' but got '" + str(read, "utf-8") + "'")
    
    def _change_regex_code(self, new_regex_code: bytearray) -> None:
        """Sends the new regex code to Arduino to be loaded on CICERO memory.

        :param bytearray new_regex_code: the code of the regex
        :raises Exception: if Arduino doesn't respond
        """
        self._send_command(self.CMD_REGEX)
        
        self._send_data_length(new_regex_code)
        self._serial_write(new_regex_code)

        read = self._serial_read()
        if read != self.INPUT_TERMINATOR:
            raise Exception("Could not change regex code!")

        self.regex_loaded = True

    def load_regex(self, regex: str, regex_format: str="pythonre") -> None:
        """Compiles the regex to bytecode and loads it to CICERO memory.

        :param str regex: the new regex to load
        :param str regex_format: the format of the regex (see compiler), defaults to "pythonre"
        """
        regex_code = self._compile_regex(regex, regex_format)

        if self.debug:
            print("Compiled regex code: " + decode_bytes_as_hex(regex_code))

        self._change_regex_code(regex_code)

    def load_string_and_start(self, string: str|bytes) -> None:
        """Loads a new string to CICERO memory and starts the execution.

        :param str|bytes string: the new string to load
        :raises Exception: if there is no regex loaded, the driver is not in text mode or Arduino doesn't respond
        """
        if not self.regex_loaded:
            raise Exception("Trying to load a string before loading the regex")
        if self.driver_status != self.DriverStatus.TEXT_MODE:
            raise Exception("Trying to load a string while not in text mode")
        
        self._send_data_length(string)
        # If string is a bytestring do not try to decode it as utf-8
        if isinstance(string, str):
            self._serial_write(string, "utf-8")
        else:
            self._serial_write(string)

        read = self._serial_read()
        if read != self.INPUT_TERMINATOR:
            raise Exception("Could not load string!")
        
        self.driver_status = self.DriverStatus.EXECUTION_MODE
        
    def wait_result(self) -> tuple[str,int]:
        """Waits for the result of a computation and return the parsed values.

        :raises Exception: if the result is not valid
        :return tuple[str,int]: result and clock cycles elapsed for this execution
        """
        result = self._serial_read()
        if result not in [self.MATCH_FOUND.encode("utf-8"), self.MATCH_NOT_FOUND.encode("utf-8"), self.CICERO_ERROR.encode("utf-8")]:
            raise Exception("Invalid result: " + decode_bytes_as_hex(result))

        elapsedCC = self._serial_read_until_terminator()
              
        # Convert from bytestring to int
        elapsedCC = int(elapsedCC.decode("utf-8"))
        
        self.driver_status = self.DriverStatus.TEXT_MODE
        return result.decode("utf-8"), elapsedCC
    
    def load_regex_and_test_strings(self, regex: str, strings: list[str], regex_format="pythonre") -> list[tuple[bool,int,float]]:
        """Loads a regex and test all the given strings on it.

        :param str regex: the regex to load
        :param list[str] strings: a list of strings to test
        :param str regex_format: format of the regex (see compiler), defaults to "pythonre"
        :return list[tuple[bool,int,float]]: a list of tuples containing the following for each tested string: if a match was found, the elapsed clock cycles and the estimated execution time for that computation
        """
        results = []

        if self.debug:
            print("Loading regex: " + regex)

        self.load_regex(regex, regex_format)

        self._enter_text_mode()
        for string in strings:           
            if self.debug:
                print("Loading string: " + string)

            self.load_string_and_start(string)
            
            result, elapsedCC = self.wait_result()
            if result == self.CICERO_ERROR:
                print("WARN: CICERO error on regex: ", regex, ", string: ", string)
            
            # Estimate exec time (in seconds) based on CICERO's clock frequency
            execTime = elapsedCC / self.CICERO_CLOCK_FREQ
            # Convert to microseconds
            execTime = execTime*1e6
            # Convert result to boolean
            result = result == self.MATCH_FOUND

            # Import model to test CICERO's results against (folder has been added to PATH)
            import golden_model
            golden_model_res = golden_model.get_golden_model_result(regex, string, no_prefix=False, no_postfix=False, frontend=regex_format)
            if result != golden_model_res:
               print("CICERO output (" + str(result) + ") is incorrect for regex '" + regex + "', string '" + string + "'")

            results.append([result, elapsedCC, execTime])
        self._exit_text_mode()

        return results
