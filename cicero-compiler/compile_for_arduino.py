import re2compiler


def code_to_bytes(code):
    res = bytearray()
    
    for line in code:
        if line.lstrip() == '':
            break
        tmp = int(line, 16)
        res += tmp.to_bytes(2, 'big')
    return res

data = 'a(b|c)*'
code = re2compiler.compile(data=data, O1=True)
code = code.split('\n')
print(code)
code_bytes = code_to_bytes(code)

with open('code.h', 'w') as f:
    numbytes = len(code_bytes)
    numbytes_bytes = numbytes.to_bytes(2, 'big')
    for i in range(0, 2):
        f.write(str(int.from_bytes(numbytes_bytes[i:i+1], 'big')) + ',')
    for i in range(0, numbytes):
        if i == numbytes - 1:
            f.write(str(int.from_bytes(code_bytes[i:i+1], 'big')))
        else:
            f.write(str(int.from_bytes(code_bytes[i:i+1], 'big')) + ',')
