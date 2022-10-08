from CiceroSerial.driver import CiceroOnArduino


def print_results(results):
    for r, elapsedCC, execTime in results:
        res_str = ""
        if r == CiceroOnArduino.MATCH_FOUND:
            res_str = "OK"
        elif r == CiceroOnArduino.MATCH_NOT_FOUND:
            res_str = "KO"
        else:
            res_str = "ERR"
        print(res_str + " - " + str(execTime) + "s (" + str(elapsedCC) + "cc)")


if __name__ == "__main__":
    cicero = CiceroOnArduino("../cicero_compiler", debug=True, timeout=5)
    
    res = cicero.load_regex_and_test_strings("a+(b|c)+", ["aaab", "fdkllwk", "njfkdackljldg", "lkortioe", "jkgjdfaaabc"])
    print_results(res)
    
    print()
    
    res = cicero.load_regex_and_test_strings("g+h+", ["aaab", "fdklgghlwk", "njfkdackljldg", "lkortioeggggh", "jkgjdfaaabc"])
    print_results(res)
    
    print()
    
    res = cicero.load_regex_and_test_strings("8[a-z]+9", ["aaab", "fdkl8ggh9lwk", "njfkda8ckljldg9", "lkorti89oeggggh", "8jkgjd9faaabc"])
    print_results(res)
