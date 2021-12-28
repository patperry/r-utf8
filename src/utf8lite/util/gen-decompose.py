#!/usr/bin/env python3

# Copyright 2017 Patrick O. Perry.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import operator
import math

try:
    import unicode_data
except ModuleNotFoundError:
    from util import unicode_data

decomp_vals = unicode_data.decomp_vals
decomp_map = unicode_data.decomp_map
decomp = unicode_data.decomp

def compute_tables(block_size):
    nblock = len(decomp) // block_size
    stage1 = [None] * nblock
    stage2 = []
    stage2_dict = {}
    for i in range(nblock):
        begin = i * block_size
        end = begin + block_size
        block = tuple(decomp[begin:end])
        if block in stage2_dict:
            j = stage2_dict[block]
        else:
            j = len(stage2)
            stage2_dict[block] = j
            stage2.append(block)
        stage1[i] = j
    return (stage1,stage2)


def stage1_item_size(nstage2):
    nbyte = math.ceil(math.log(nstage2, 2) / 8)
    size = 2**math.ceil(math.log(nbyte, 2))
    return size

page_size = 4096
block_size = 256

nbytes = {}

best_block_size = 1
smallest_size = len(decomp)

for i in range(1,17):
    block_size = 2**i
    stage1,stage2 = compute_tables(block_size)

    nbyte1 = len(stage1) * stage1_item_size(len(stage2))
    nbyte2 = len(stage2) * block_size

    nbyte1 = math.ceil(nbyte1 / page_size) * page_size
    nbyte2 = math.ceil(nbyte2 / page_size) * page_size
    nbyte = nbyte1 + nbyte2
    nbytes[block_size] = nbyte

    if nbyte < smallest_size:
        smallest_size = nbyte
        best_block_size = block_size


block_size = best_block_size
stage1,stage2 = compute_tables(block_size)

type1_size = stage1_item_size(len(stage2))
if type1_size == 1:
    type1 = 'uint8_t'
elif type1_size == 2:
    type1 = 'uint16_t'
elif type1_size == 4:
    type1 = 'uint32_t'
else:
    type1 = 'uint64_t'

# Write decompose.h to stdout

print("/* This file is automatically generated. DO NOT EDIT!")
print("   Instead, edit gen-decompose.py and re-run.  */")
print("")
print("/*")
print(" * Decomposition mappings.")
print(" *")
print(" * Defined in UAX #44 \"Unicode Character Database\"")
print(" *")
print(" *     http://www.unicode.org/reports/tr44/")
print(" *")
print(" * Section 5.7.3, Table 14.")
print(" *")
print(" *")
print(" * We use a two-stage lookup strategy as described at")
print(" *")
print(" *     http://www.strchr.com/multi-stage_tables")
print(" *")
print(" */")
print("")
print("#ifndef UNICODE_DECOMPOSE_H")
print("#define UNICODE_DECOMPOSE_H")
print("")
print("#include <stdint.h>")
print("")
print("/* decomposition_type")
print(" * ------------------")
print(" * compatibility decompositions have decomposition_type != 0")
print(" */")
print("enum decomposition_type {", end="")
first = True
for k,v in sorted(decomp_vals.items(), key=operator.itemgetter(1)):
    if not first:
        print(",", end="")
    print("\n\tDECOMPOSITION_" + k.upper() + " = " + str(v), end="")
    first = False
print("\n};")
print("")
print("/* decomposition")
print(" * -------------")
print(" * type:   the decomposition_type")
print(" *")
print(" * length: the length (in codepoints) of the decomposition mapping,")
print(" *         or 0 if there is no decomposition")
print(" *")
print(" * data:   the mapped-to codepoint (length = 1), or")
print(" *         an index into the `decomposition_mapping` array, pointing")
print(" *         to the first codepoint in the mapping (length > 1)")
print(" */")
print("struct decomposition {")
print("\tint type : 6;")
print("\tunsigned length : 5;")
print("\tunsigned data : 21;")
print("};")
print("")
print("#define DECOMPOSITION_BLOCK_SIZE", block_size)
print("")
print("static const " + type1 + " decomposition_stage1[] = {")
for i in range(len(stage1) - 1):
    if i % 16  == 0:
        print("/* U+{:04X} */".format(i * block_size), end="")
    print("{0: >3},".format(stage1[i]), end="")
    if i % 16 == 15:
        print("")
print("{0: >3}".format(stage1[len(stage1) - 1]))
print("};")
print("")
print("static const struct decomposition decomposition_stage2[][" +
        str(block_size) + "] = {")
for i in range(0,len(stage2)):
    print("  /* block " + str(i) + " */")
    print("  {", end="")
    for j in range(block_size):
        val = stage2[i][j]
        if val is None:
            print("{0,0,0}", end="")
        else:
            if val[0] is None:
                t = 0
            else:
                t = decomp_vals[val[0]]
            print("{{{0},{1},0x{2:05X}}}".format(t, val[1], val[2]), end="")

        #print("{0: >3}".format(prop_vals[stage2[i][j]]), end="")
        if j + 1 == block_size:
            print("\n  }", end="")
        else:
            print(",", end="")
            if j % 5 == 4:
                print("\n   ", end="")
    if i + 1 != len(stage2):
        print(",\n")
    else:
        print("")
print("};")
print("")
print("static const int32_t decomposition_mapping[] = {")
for i in range(len(decomp_map) - 1):
    if i % 8  == 0:
        print("/* 0x{:04X} */ ".format(i), end="")
    print("0x{0:04X},".format(decomp_map[i]), end="")
    if i % 8 == 7:
        print("")
print("0x{0:04X}".format(decomp_map[len(decomp_map) - 1]))
print("};")
print("")
print("#endif /* UNICODE_DECOMPOSE_H */")