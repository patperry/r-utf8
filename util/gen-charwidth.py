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

import math

try:
    import property
    import unicode_data
except ModuleNotFoundError:
    from util import property
    from util import unicode_data


DERIVED_CORE_PROPERTIES = "data/ucd/DerivedCoreProperties.txt"
EAST_ASIAN_WIDTH = "data/ucd/EastAsianWidth.txt"
EMOJI_DATA = "data/emoji/emoji-data.txt"


east_asian_width = property.read(EAST_ASIAN_WIDTH)
# A:  Ambiguous (can be narrow or wide depending on context; treat as wide)
# F:  Fullwidth (always wide)
# H:  Halfwidth (always narrow)
# Na: Narrow    (always narrow)
# N:  Neutral   (non-East Asian; treat as single)
# W:  Wide      (always wide)


emoji_props = property.read(EMOJI_DATA, sets=True)

# text presentation (MacOS)
emoji_text = set([
    # number sign, asterisk
    0x0023, 0x002A,

    # digit zero..digit nine
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034,
    0x0035, 0x0036, 0x0037, 0x0038, 0x0039,

    # copyright, reserved
    0x00A9, 0x00AE,

    # double exclamation, exclamation question mark
    0x203C, 0x2049,

    # trade mark
    0x2122,

    # arrows
    0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199,

    # black, white small square
    0x25AA, 0x25AB,

    # play, reverse button
    0x25B6, 0x25C0,

    # male, female sign
    0x2640, 0x2642,

    # spade, club, heart, diamond suit
    0x2660, 0x2663, 0x2665, 0x2666
    ])

# emoji presentation
emoji = set()
for code in emoji_props['Emoji']:
    if code not in emoji_text:
        emoji.add(code)

# The following makes more sense according to the spec, but doesn't catch
# all presentation emoji (at least, not on MacOS):
# emoji = emoji_props['Emoji_Presentation']
# for code in emoji_props['Emoji_Modifier_Base']:
#     emoji.add(code)

# Treat ignorables as invisible
derived_core_properties = property.read(DERIVED_CORE_PROPERTIES, sets=True)
default_ignorable = derived_core_properties['Default_Ignorable_Code_Point']


# unassigned: not assigned, other, surrogate
none_cats = set(['Cc', 'Cn', 'Co', 'Cs', 'Zl', 'Zp'])
mark_cats = set(['Cf', 'Me', 'Mn'])
none = set([0xFFF9, 0xFFFA, 0xFFFB]) # interlinear annotation markers
mark = set()
for code in range(len(unicode_data.uchars)):
    u = unicode_data.uchars[code]
    if code in none or code in mark:
        pass
    elif u is None or u.category in none_cats:
        none.add(code)
    elif u.category in mark_cats:
        mark.add(code)


code_props = [None] * len(east_asian_width)
for code in range(len(code_props)):
    eaw = east_asian_width[code]
    if code in default_ignorable: # default ingorable overrides
        code_props[code] = 'Ignorable'
    elif code in emoji: # emoji overrides
        code_props[code] = 'Emoji'
    elif code in mark: # mark overrides
        code_props[code] = 'Mark'
    elif code in none: # none overrides
        code_props[code] = 'None'
    elif eaw == 'F' or eaw == 'W':
        code_props[code] = 'Wide'
    elif eaw == 'H' or eaw == 'Na' or eaw == 'N':
        code_props[code] = 'Narrow'
    elif eaw == 'A':
        code_props[code] = 'Ambiguous'
    else:
        code_props[code] = 'Narrow' # default to narrow


prop_names = [
        'None', 'Ignorable', 'Mark', 'Narrow', 'Ambiguous', 'Wide', 'Emoji'
        ]
prop_vals = {}
for p in prop_names:
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


# Write chardwidth.h to stdout

print("/* This file is automatically generated. DO NOT EDIT!")
print("   Instead, edit gen-charwidth.py and re-run.  */")
print("")
print("/*")
print(" * Unicode East_Asian_Width property values.")
print(" *")
print(" * Defined in UAX #11 \"East Asian Width\"")
print(" *")
print(" *     http://www.unicode.org/reports/tr11/")
print(" *")
print(" * We use the two-stage lookup strategy described at")
print(" *")
print(" *     http://www.strchr.com/multi-stage_tables")
print(" *")
print(" */")
print("")
print("#ifndef CHARWIDTH_H")
print("#define CHARWIDTH_H")
print("")
print("#include <stdint.h>")
print("")
print("enum charwidth_prop {")
first = True
for prop in prop_names:
    if not first:
        print(",\n", end="")
    else:
        first = False
    print("\tCHARWIDTH_" + prop.upper() + " = " + str(prop_vals[prop]), end="")
print("\n};")
print("")
print("static const " + type1 + " charwidth_stage1[] = {")
for i in range(len(stage1) - 1):
    if i % 16  == 0:
        print("/* U+{:04X} */".format(i * block_size), end="")
    print("{0: >3},".format(stage1[i]), end="")
    if i % 16 == 15:
        print("")
print("{0: >3}".format(stage1[len(stage1) - 1]))
print("};")
print("")
print("static const " + type2 + " charwidth_stage2[][" +
        str(block_size) + "] = {")
for i in range(len(stage2)):
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
print("static int charwidth(int32_t code)")
print("{")
print("\tconst int32_t block_size = " + str(block_size) + ";")
print("\t" + type1 + " i = charwidth_stage1[code / block_size];")
print("\treturn charwidth_stage2[i][code % block_size];")
print("}")
print("")
print("#endif /* CHARWIDTH_H */")
