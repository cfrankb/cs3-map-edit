#!/bin/python

import glob
import binascii
from os.path import basename

CHUNK_SIZE = 32

def chunks(l, n):
    for i in range(0, len(l), n):
        yield l[i:i+n]


defs = ['#include <stdint.h>', '', '#pragma once', '', ]

with open('src/data.cpp', 'wb') as tfile:
    i =0
    tfile.write('#include <cstdint>\n'.encode('utf-8'))
    for fname in glob.glob('data/*'):
        if fname.endswith('.py'):
            continue
        if fname.find('/old') != -1:
            continue
        sfile = open(fname, 'rb')
        data = sfile.read()
        symbol = basename(fname).replace('.rle', '').replace('.', "_")
        print(fname)
        if fname.endswith('.mcz'):
            # strip header from mcz file
            data = data[12:]        
        defs.append(f'extern uint8_t {symbol}[{len(data)}];')
        sfile.close()
        tfile.write(f'uint8_t {symbol}[]={{\n'.encode('utf-8'))
        for line in chunks(binascii.b2a_hex(data), CHUNK_SIZE):
            s = '    ' + ','.join(['0x'+ch.decode("utf-8") for ch in chunks(line, 2)])
            tfile.write(s.encode('utf-8'))
            if len(line) == CHUNK_SIZE:
                tfile.write(','.encode('utf-8'))
            tfile.write('\n'.encode('utf-8'))
        tfile.write('};\n\n'.encode('utf-8'))
        i += 1


#// extern uint8_t tiles_mcz;
with open('src/data.h', 'wb') as tfile:
    t = '\n'.join(defs)
    tfile.write(t.encode('utf-8'))