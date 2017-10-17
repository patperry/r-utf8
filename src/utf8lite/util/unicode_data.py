# unicode_data

import collections
import re


UNICODE_DATA = 'data/ucd/UnicodeData.txt'
UNICODE_MAX = 0x10FFFF

try:
    unicode_data = open(UNICODE_DATA, "r")
except FileNotFoundError:
    unicode_data = open("../" + UNICODE_DATA, "r")

field_names = [
    'name',     # Name
    'category', # General_Category
    'ccc',      # Canonical_Combining_Class
    'bidi',     # Bidi_Class
    'decomp',   # Decomposition_Type, Decomposition_Mapping
    'decimal',  # Numeric_Value (Numeric_Type = Decimal)
    'digit',    # Numeric_Value (Numeric_Type = Decimal, Digit)
    'numeric',  # Numeric_Value (Numeric_Type = Decimal, Digit, Numeric)
    'mirrored', # Bidi_Mirrored
    'old_name', # Unicode_1_Name
    'comment',  # ISO_Comment
    'ucase',    # Simple_Uppercase_Mapping
    'lcase',    # Simple_Lowercase_Mapping
    'tcase'     # Simple_Titlecase_Mapping
    ]

UChar = collections.namedtuple('UChar', field_names)
ids = UChar._make(range(1, len(field_names) + 1))

Decomp = collections.namedtuple('Decomp', ['type', 'map'])

DECOMP_PATTERN = re.compile(r"""^(<(\w+)>)?\s* # decomposition type
                                 ((\s*[0-9A-Fa-f]+)+) # decomposition mapping
                                 \s*$""", re.X)
RANGE_PATTERN = re.compile(r"""^<([^,]+),\s*    # range name
                                 (First|Last) # first or last
                                 >$""", re.X)

def parse_decomp(code, field):
    if field != '':
        m = DECOMP_PATTERN.match(field)
        assert m
        d_type = m.group(2)
        d_map = tuple([int(x, 16) for x in m.group(3).split()])
        return Decomp(type=d_type, map=d_map)

    elif code in range(0xAC00, 0xD7A4):
        return Decomp(type='hangul', map=None)
    else:
        return None


def parse_code(field):
    if field != '':
        return int(field, 16)
    else:
        return None

def parse_int(field):
    if field != '':
        return int(field)
    else:
        return None

def parse_str(field):
    if field == '':
        return None
    else:
        return field


uchars = [None] * (UNICODE_MAX + 1)

with unicode_data:
    for line in unicode_data:
        fields = line.strip().split(';')
        code = int(fields[0], 16)
        uchars[code] = UChar(name = fields[ids.name],
                             category = parse_str(fields[ids.category]),
                             ccc = parse_int(fields[ids.ccc]),
                             bidi = fields[ids.bidi],
                             decomp = parse_decomp(code, fields[ids.decomp]),
                             decimal = fields[ids.decimal],
                             digit = fields[ids.digit],
                             numeric = fields[ids.numeric],
                             mirrored = fields[ids.mirrored],
                             old_name = fields[ids.old_name],
                             comment = fields[ids.comment],
                             ucase = parse_code(fields[ids.ucase]),
                             lcase = parse_code(fields[ids.lcase]),
                             tcase = parse_code(fields[ids.tcase]))


utype = None

for code in range(len(uchars)):
    u = uchars[code]
    if u is None:
        uchars[code] = utype
    else:
        m = RANGE_PATTERN.match(u.name)
        if m:
            if m.group(2) == 'First':
                utype = u._replace(name = '<' + m.group(1) + '>')
            else:
                utype = None



decomp_vals = {
    'hangul': -1, 'none': 0,
    'font': 1, 'noBreak': 2, 'initial': 3, 'medial': 4, 'final': 5,
    'isolated': 6, 'circle': 7, 'super': 8, 'sub': 9, 'vertical': 10,
    'wide': 11, 'narrow': 12, 'small': 13, 'square': 14, 'fraction': 15,
    'compat': 16 }

decomp_map = []
decomp = []

for code in range(len(uchars)):
    u = uchars[code]

    if u is None or u.decomp is None:
        decomp.append(None)
        continue

    d = u.decomp
    if d.map is not None:
        d_len = len(d.map)

        if d_len > 1:
            d_data = len(decomp_map)
            decomp_map.extend(d.map)
        else:
            d_data = d.map[0]

        decomp.append((d.type, d_len, d_data))

    elif d.type == 'hangul':
        decomp.append(('hangul', 2, 0))

    else:
        decomp.append(None)


# From Unicode-8.0 Section 3.12 Conjoining Jamo Behavior
HANGUL_SBASE = 0xAC00
HANGUL_LBASE = 0x1100
HANGUL_VBASE = 0x1161
HANGUL_TBASE = 0x11A7
HANGUL_LCOUNT = 19
HANGUL_VCOUNT = 21
HANGUL_TCOUNT = 28
HANGUL_NCOUNT = (HANGUL_VCOUNT * HANGUL_TCOUNT)
HANTUL_SCOUNT = (HANGUL_LCOUNT * HANGUL_NCOUNT)


def hangul_decompose(code):
    sindex = code - HANGUL_SBASE
    lindex = sindex // HANGUL_NCOUNT
    vindex = (sindex % HANGUL_NCOUNT) // HANGUL_TCOUNT
    tindex = sindex % HANGUL_TCOUNT
    lpart = HANGUL_LBASE + lindex
    vpart = HANGUL_VBASE + vindex;
    tpart = HANGUL_TBASE + tindex;
    if tindex > 0:
        return (lpart, vpart, tpart)
    else:
        return (lpart, vpart)


# get a character's decomposition if one exists, as a tuple
def decompose(code, compat=True):
    dc = decomp[code]
    if dc is None:
        return None
    t = dc[0]
    l = dc[1]
    if not compat and t is not None and t != 'hangul':
        return None
    if l == 1:
        return (dc[2],)
    elif dc[0] != 'hangul':
        o = dc[2]
        return tuple(decomp_map[o:o + l])
    else:
        return hangul_decompose(code)
