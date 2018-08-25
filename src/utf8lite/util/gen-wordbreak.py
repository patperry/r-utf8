#!/usr/bin/env python3

# Copyright 2016 Patrick O. Perry.
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

import math

try:
    import property
    import unicode_data
except ModuleNotFoundError:
    from util import property
    from util import unicode_data


WORD_BREAK_PROPERTY = "data/ucd/auxiliary/WordBreakProperty.txt"
PROP_LIST = "data/ucd/PropList.txt"
DERIVED_CORE_PROPERTIES = "data/ucd/DerivedCoreProperties.txt"

code_props = property.read(WORD_BREAK_PROPERTY)
word_break_property = property.read(WORD_BREAK_PROPERTY, sets=True)

prop_list = property.read(PROP_LIST, sets=True)
white_space = prop_list['White_Space']

derived_core_properties = property.read(DERIVED_CORE_PROPERTIES, sets=True)
default_ignorable = derived_core_properties['Default_Ignorable_Code_Point']

# add the default ignorables to the white space category
white_space = white_space.union(default_ignorable)

letter = set()
#mark = set()
number = set()
other = set()
punctuation = set()
symbol = set()
letter_cats = set(('Ll', 'Lm', 'Lo', 'Lt', 'Lu', 'Nl'))
#mark_cats = set(('Mc', 'Me', 'Mn'))
number_cats = set(('Nd', 'No')) # Note: Nl in 'letter'
other_cats = set(('Cc', 'Cf', 'Cs', 'Co', 'Cn'))
punctuation_cats = set(('Pc', 'Pd', 'Pe', 'Pf', 'Pi', 'Po', 'Ps'))
symbol_cats = set(('Sc', 'Sk', 'Sm', 'So'))

for code in range(len(unicode_data.uchars)):
    u = unicode_data.uchars[code]
    if u is None or u.category in other_cats:
        other.add(code)
    elif u.category in letter_cats:
        letter.add(code)
#    elif u.category in mark_cats:
#        mark.add(code)
    elif u.category in number_cats:
        number.add(code)
    elif u.category in punctuation_cats:
        punctuation.add(code)
    elif u.category in symbol_cats:
        symbol.add(code)

# reclassify legacy punctuation as 'Symbol'
for ch in ['#', '%', '&', '@']:
    punctuation.remove(ord(ch))
    symbol.add(ord(ch))
    # fullwidth versions
    wch =  0xFEE0 + ord(ch)
    punctuation.remove(wch)
    symbol.add(wch)


prop_names = set(code_props)
prop_names.remove(None)

assert 'Letter' not in prop_names
assert 'Mark' not in prop_names
assert 'Number' not in prop_names
assert 'Other' not in prop_names
assert 'Punctuation' not in prop_names
assert 'Symbol' not in prop_names
assert 'White_Space' not in prop_names
prop_names.add('Letter')
prop_names.add('Number')
#prop_names.add('Mark')
prop_names.add('Other')
prop_names.add('Punctuation')
prop_names.add('Symbol')
prop_names.add('White_Space')

for code in range(len(code_props)):
    if code_props[code] is None:
        if code in white_space:
            code_props[code] = 'White_Space'
        elif code in letter:
            code_props[code] = 'Letter'
#        elif code in mark:
#            code_props[code] = 'Mark'
        elif code in number:
            code_props[code] = 'Number'
        elif code in other:
            code_props[code] = 'Other'
        elif code in punctuation:
            code_props[code] = 'Punctuation'
        elif code in symbol:
            code_props[code] = 'Symbol'

# override the hyphen property (default is 'Punctuation')
prop_names.add('Hyphen')
code_props[0x002D] = 'Hyphen' # HYPHEN-MINUS
code_props[0x058A] = 'Hyphen' # ARMENIAN HYPHEN
code_props[0x05BE] = 'Hyphen' # HEBREW PUNCTUATION MAQAF
code_props[0x1400] = 'Hyphen' # CANADIAN SYLLABICS HYPHEN
code_props[0x1806] = 'Hyphen' # MONGOLIAN TODO SOFT HYPHEN
code_props[0x2010] = 'Hyphen' # HYPHEN
code_props[0x2011] = 'Hyphen' # NON-BREAKING HYPHEN
code_props[0x2E17] = 'Hyphen' # DOUBLE OBLIQUE HYPHEN
code_props[0x2E1A] = 'Hyphen' # HYPHEN WITH DIAERESIS
code_props[0x2E40] = 'Hyphen' # DOUBLE HYPHEN
code_props[0x30A0] = 'Hyphen' # KATAKANA-HIRAGANA DOUBLE HYPHEN
code_props[0xFE63] = 'Hyphen' # SMALL HYPHEN-MINUS
code_props[0xFF0D] = 'Hyphen' # FULLWIDTH HYPHEN-MINUS

# extra MidLetter properties
# TR#29: "Some or all of the following characters may be tailored to be in
#   MidLetter, depending on the environment: 
code_props[0x055A] = 'MidLetter' # ARMENIAN APOSTROPHE
code_props[0x0F0B] = 'MidLetter' # TIBETAN MARK INTERSYLLABIC TSHEG
code_props[0x201B] = 'MidLetter' # SINGLE HIGH-REVERSED-9 QUOTATION MARK
code_props[0x30FB] = 'MidLetter' # KATAKANA MIDDLE DOT

# make sure we didn't miss anything
for code in range(len(code_props)):
    if code_props[code] is None:
        u = unicode_data.uchars[code]
        print('Uncagetorized code point:')
        print('U+{:04X}'.format(code), u.category, u.name)
        assert False

prop_vals = {}
prop_vals['None'] = 0;
for p in sorted(prop_names):
    prop_vals[p] = len(prop_vals)


def compute_tables(block_size):
    nblock = len(code_props) // block_size
    stage1 = [None] * nblock
    stage2 = []
    stage2_dict = {}
    for i in range(nblock):
        begin = i * block_size
        end = begin + block_size
        block = tuple(code_props[begin:end])
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
smallest_size = len(code_props)

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

type2 = 'int8_t'


# Write wordbreakprop.h to stdout

print("/* This file is automatically generated. DO NOT EDIT!")
print("   Instead, edit gen-wordbreak.py and re-run.  */")
print("")
print("/*")
print(" * Unicode Word_Break property values.")
print(" *")
print(" * Defined in UAX #29 \"Unicode Text Segmentation\"")
print(" *")
print(" *     http://www.unicode.org/reports/tr29/")
print(" *")
print(" * Section 4.1, Table 3.")
print(" *")
print(" *")
print(" * We use the two-stage lookup strategy described at")
print(" *")
print(" *     http://www.strchr.com/multi-stage_tables")
print(" *")
print(" */")
print("")
print("#ifndef WORDBREAKPROP_H")
print("#define WORDBREAKPROP_H")
print("")
print("#include <stdint.h>")
print("")
print("enum word_break_prop {")
print("\tWORD_BREAK_NONE = 0", end="")
for prop in sorted(prop_names):
    print(",\n\tWORD_BREAK_" + prop.upper() + " = " + str(prop_vals[prop]),
          end="")
print("\n};")
print("")
print("static const " + type1 + " word_break_stage1[] = {")
for i in range(len(stage1) - 1):
    if i % 16  == 0:
        print("/* U+{:04X} */".format(i * block_size), end="")
    print("{0: >3},".format(stage1[i]), end="")
    if i % 16 == 15:
        print("")
print("{0: >3}".format(stage1[len(stage1) - 1]))
print("};")
print("")
print("static const " + type2 + " word_break_stage2[][" +
        str(block_size) + "] = {")
#for i in range(len(stage2)):
for i in range(0,len(stage2)):
    print("  /* block " + str(i) + " */")
    print("  {", end="")
    for j in range(block_size):
        print("{0: >3}".format(prop_vals[stage2[i][j]]), end="")
        if j + 1 == block_size:
            print("\n  }", end="")
        else:
            print(",", end="")
            if j % 16 == 15:
                print("\n   ", end="")
    if i + 1 != len(stage2):
        print(",\n")
    else:
        print("")
print("};")

print("")
print("static int word_break(int32_t code)")
print("{")
print("\tconst int32_t block_size = " + str(block_size) + ";")
print("\t" + type1 + " i = word_break_stage1[code / block_size];")
print("\treturn word_break_stage2[i][code % block_size];")
print("}")
print("")
print("#endif /* WORDBREAKPROP_H */")
