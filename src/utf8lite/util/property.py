
import re

PATTERN = re.compile(r"""^([0-9A-Fa-f]+)        # (first code)
                          (\.\.([0-9A-Fa-f]+))? # (.. last code)?
                          \s*
                          ;                     # ;
                          \s*
                          (\w+)                 # (property name)
                          \s*
                          (\#.*)?$              # (# comment)?""", re.X)

UNICODE_MAX = 0x10FFFF


def read(filename, sets=False):
    try:
        file = open(filename, "r")
    except FileNotFoundError:
        file = open("../" + filename, "r")
    
    code_props = [None] * (UNICODE_MAX + 1)
    prop_names = set()
    properties = {}

    with file:
        for line in file:
            line = line.split("#")[0] # remove comment
            m = PATTERN.match(line)
            if m:
                first = int(m.group(1), 16)
                if m.group(3):
                    last = int(m.group(3), 16)
                else:
                    last = first
                name = m.group(4)
                if not name in properties:
                    properties[name] = set()
                prop = properties[name]
                for u in range(first, last + 1):
                    if not sets:
                        assert code_props[u] is None
                    code_props[u] = name
                    prop.add(u)
                prop_names.add(name)
    if sets:
        return properties
    else:
        return code_props
