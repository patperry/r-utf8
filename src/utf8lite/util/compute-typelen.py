#!/usr/bin/env python3

import math
import re

CASE_FOLDING = 'data/ucd/CaseFolding.txt'
UNICODE_MAX = 0x10FFFF

# Parse CaseFolding.txt

try:
    file = open(CASE_FOLDING, 'r')
except FileNotFoundError:
    file = open('../' + CASE_FOLDING, 'r')

def utf8_len(code):
    if code <= 0x7f:
        return 1
    elif code <= 0x07FF:
        return 2
    elif code <= 0xFFFF:
        return 3
    else:
        return 4

with file:
    for line in file:
        if line[0] != '#' and line[0] != '\n':
            fields = line.split(';')
            code = int(fields[0], 16)
            status = fields[1].strip();

            if status == 'C' or status == 'F':
                l0 = utf8_len(code)

                mapping = [int(x,16) for x in fields[2].split()]
                l1 = sum([utf8_len(m) for m in mapping])

                ratio = l1 / l0
                if ratio >= 3:
                    print('U+{:04X}'.format(code), mapping, 'ratio: ', ratio)

