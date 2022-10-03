import serial
import time

arduino = serial.Serial(port='COM3', baudrate=9600, timeout=.1)

CMD_REGEX = b"\x00"
CMD_TEXT = b"\x01"

INPUT_TERMINATOR = b"\xFF"

MATCH_FOUND = "2"
MATCH_NOT_FOUND = "3"

in_text_mode = False

def serial_write(data, encoding=None):
    if encoding:
        arduino.write(bytes(data, encoding))
    else:
        arduino.write(bytes(data))
    
def change_regex(new_regex_code):
    serial_write(CMD_REGEX)
    serial_write(new_regex_code)
    serial_write(INPUT_TERMINATOR)

def enter_text_mode():
    global in_text_mode
    if not in_text_mode:
        serial_write(CMD_TEXT)
        in_text_mode = True

def load_string_and_start(string):
    if not in_text_mode:
        return
    
    serial_write(string, "utf-8")
    serial_write("\n", "utf-8")
    
def exit_text_mode():
    global in_text_mode
    if in_text_mode:
        serial_write(INPUT_TERMINATOR)
        in_text_mode = False

def wait_result():
    result = arduino.read()
    return result.decode("utf-8")

def compile_regex(regex):
    # TODO: implement
    return regex

def compile_regex(regex):
    import sys

    # Add Cicero compiler folder to path
    sys.path.append('../cicero_compiler')

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
    
def batch_test(regex, strings):
    results = []
    
    regex_code = compile_regex(regex)
    change_regex(regex_code)
    
    time.sleep(0.05)
    
    enter_text_mode()    
    for string in strings:
        load_string_and_start(string)
        results += wait_result()
    
    exit_text_mode()
    
    return results

if __name__ == "__main__":
    res = batch_test("a+(b|c)+", ["aaab", "fdkllwk", "njfkdackljldg", "lkortioe", "jkgjdfaaabc"])
    for r in res:
        if r == MATCH_FOUND:
            print("OK")
        elif r == MATCH_NOT_FOUND:
            print("KO")
        else:
            print("ERR")
    
