#!/bin/env python2

# as.py -- Assembler
#
# Simple Python script to convert Khepra assembly programs into
# working ROMs loadable by qpra and other emulators as per the specification.

import sys
import re
import struct

anyPattern = r"""
([a-zA-Z0-9_-]+:)?
(?:.+)?
$
"""

instrPattern = r"""
([a-zA-Z0-9_-]+:)?         # Label, optional
\s*
([a-z]+)([.][bw])?      # Instruction, required; and b/w suffix, optional
\s*
(\[?\$?\w+\]?)?,?       # First operand, optional, with comma
\s*
(\[?\$?\w+\]?)?         # Second operand, optional, with comma
\s*
(;.+)?                  # Comment, optional
\s*
$                       # Line end
"""

dirPattern = r"""
([.]\w+)
\s*
(\$?\w+)?,?\s*(\$?\w+)?,?\s*(\$?\w+)?,?\s*(\$?\w+)?,?\s*
$
"""

defs = {}

directives = { ".bank":0, ".db":1, ".org":3 }

banks = {
        "rom_fixed":0, "rom_swap":1, "ram_fixed":2,
        "ram_swap":3, "tile_swap":4, "audio_swap":5
        }

bank_addrs = {
        0:0x0000, 1:0x4000, 2:0x8000, 3:0xa000, 4:0xc000, 5:0xf000
        }

bank_sizes = {
        0:0x4000, 1:0x4000, 2:0x2000, 3:0x2000, 4:0x2000, 5:0x0800
        }

opcodes = {
        "nop":0, "int":1, "rti":2, "rts":3, "jp":4, "cl":5, "jz":6, "cz":7,
        "jc":8, "cc":9, "jo":10, "co":11, "jn":12, "cn":13, "not":14, "inc":15,
        "dec":16, "ind":17, "ded":18, "mv":19, "cmp":20, "tst":21, "add":22,
        "sub":23, "mul":24, "div":25, "lsl":26, "lsr":27, "asr":28, "and":29,
        "or":30, "xor":31
        }
rr = ["a","b","c","d","e","f"]

regs = { 'a':0, 'b':1, 'c':2, 'd':3, 'e':4, 'f':5, 'p':6, 's':7 }


def isNum(s):
    try:
        if isinstance(s, (int, long)):
            return True
        v = re.findall("\$?(?:[0-9a-fA-F]+)", s)
        if len(v) > 0 and v[0][0] == '$':
            return True
        return False
    except ValueError:
        return False

def num(s):
    try:
        if isinstance(s, (int, long)):
            return s
        v = re.findall("\$?(?:[0-9a-fA-F]+)", s)
        if len(v) > 0 and v[0][0] == '$':
            return int(v[0][1:], 16)
        else:
            return int(s)
    except ValueError:
        return float(s)


class InvalidOperandException(Exception):
    pass

class instr:
    def __init__(self, strop, op, nops, op1="", op2="", addr=0):
        self.strop = strop
        self.op = op
        self.nops = nops
        if nops > 0:
            self.op1 = op1
        if nops > 1:
            self.op2 = op2

        self.mode = self.addrMode()

        if strop in ["nop", "int", "rti", "rts"]:
            self.size = 1
        elif nops == 1 and self.op1 in rr:
            self.size = 2
        elif nops == 2 and self.op1 in rr and self.op2 in rr:
            self.size = 2
        elif self.mode in [2,3,9,10,13]:
            self.size = 3
        else:
            self.size = 4

    def addrMode(self):
        if self.nops == 0:
            return 0
        elif self.nops == 1:
            if self.op1 in rr:
                return 0
            elif self.op1[1:-1] in rr:
                return 1
            elif isNum(self.op1):
                n = num(self.op1)
                return 2 if -128 < n < 127 else 4
            elif isNum(self.op1[1:-1]):
                n = num(self.op1[1:-1])
                return 3 if -128 < n < 127 else 5
            else:
                raise InvalidOperandException
        elif self.nops == 2:
            if self.op1 in rr:
                if self.op2 in rr:
                    return 6
                elif self.op2[1:-1] in rr:
                    return 7
                elif isNum(self.op2):
                    n = num(self.op2)
                    return 9 if -128 < n < 127 else 11
                elif isNum(self.op2[1:-1]):
                    n = num(self.op2[1:-1])
                    return 10 if -128 < n < 127 else 12
                else:
                    print opcodes.keys()[opcodes.values().index(self.op)],' ',self.op1,', ',self.op2
                    raise InvalidOperandException
            elif self.op1[1:-1] in rr:
                if self.op2 in rr:
                    return 8
                else:
                    raise InvalidOperandException
            elif isNum(self.op1[1:-1]):
                n = num(self.op[1:-1])
                if not self.op2 in rr:
                    raise InvalidOperandException
                return 13 if -128 < n < 127 else 14
            else:
                raise InvalidOperandException
        else:
            raiseInvalidOperandException

    def getAddrMode(self):
        # Implicit mode, so don't care. Return 0 as placeholder.
        if self.nops == 0:
            return 0
        if self.nops == 1:
            # `i r0` : mode 0, direct reg.
            if re.match("[abcdef]", op1) is not None:
                return 0
            # `i [r0]` : mode 1, indirect reg.
            elif re.match("\[[abcdef]\]", op1) is not None:
                return 1
            # `i $xx` or `i xxx` : mode 2, direct byte or mode 4, direct word.
            elif re.match("(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)", op1) is not None:
                if num(op1) < 256:
                    return 2
                else:
                    return 4
            # `i abcd` : mode 2, direct byte or mode 4, direct word.
            elif re.match("\w+", op1) is not None and op1 in ds:
                if num(ds[op1]) < 256:
                    return 2
                else:
                    return 4
            # `i [$xx]` or `i [xxx]` : mode 3, indirect byte offs. or mode 5, indirect word.
            elif re.match("\[(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)\]", op1) is not None:
                if -124 < (num(op1) - p) < 123:
                    return 3
                else:
                    return 5
            # `i [abcd]` : mode 3, indirect byte offs. or mode 5, indirect word
            elif re.match("\[\w+\]", op1) is not None and op1[1:-1] in ds:
                if -124 < (num(ds[op1[1:-1]]) - p) < 123:
                    return 3
                else:
                    return 5
            else:
                print 'nops1 1 error: invalid operand: ', op1
                return 0
        elif nops == 2:
            if re.match("[abcdef]", op1) is not None:
                # `i r0, r1` : mode 6, direct reg. / direct reg.
                if re.match("[abcdef]", op2) is not None:
                    return 6
                # `i r0, [r1]` : mode 7, direct reg. / indirect reg.
                elif re.match("\[[abcdef]\]", op2) is not None:
                    return 7
                # `i r0, $xx[xx]` or `i r0, xxx[xx]` : mode 9, direct reg. / direct byte or mode 11, direct reg. / direct word.
                elif re.match("(\$[abcdef0-9]{1,4}?)|(\d{1,5}?)", op2) is not None:
                    if num(op2) < 256:
                        return 9
                    else:
                        return 11
                # `i r0, abcd` : mode 9, direct reg. / direct byte or mode 11, direct reg. / direct word.
                elif re.match("\w+", op2) is not None and op2 in ds:
                    if num(ds[op2]) < 256:
                        return 9
                    else:
                        return 11
                elif re.match("\[(\$[abcdef0-9]{1,4}?)|(\d{1,5}?)\]", op2) is not None:
                    # `i r0, [$xx]` or `i r0, [xxx]` : mode 10, direct reg. / indirect byte offs.
                    if num(op2) < 256:
                        return 10
                    # `i r0, [$xxxx]` or `i r0, [xxxxx]` : mode 12, direct reg. / indirect word.
                    else:
                        return 12
                elif re.match("\[\w+\]", op2) is not None and op2[1:-1] in ds:
                    if num(ds[op2[1:-1]]) < 256:
                        return 10
                    else:
                        return 12
                else:
                    print 'nops2 1 error: invalid operand: ', op2
                    return 0
            elif re.match("\[[abcdef]\]", op1) is not None:
                # `i [r0], r1` : mode 8, indirect reg. / direct reg.
                if re.match("[abcdef]", op2) is not None:
                    return 8
                else:
                    print 'nop2 2 error: invalid operand: ', op2
                    return 0
            elif re.match("[abcdef]", op2) is not None:
                # `i [$xx[xx]], r0` : mode 13, indirect byte offs. / direct reg. or mode 14, indirect word / direct reg.
                if re.match("\[(\$[abcdef0-9]{1,4}?)|(\d{1,5}?)\]", op1) is not None:
                    if num(op1) < 256:
                        return 13
                    else:
                        return 14
                # `i [abcd], r0` : mode 13, indirect byte offs. / direct reg. or mode 14, indirect word / direct reg.
                elif re.match("\[\w+\]", op1) is not None and op1[1:-1] in ds:
                    if num(ds[op1[1:-1]]) < 256:
                        return 13
                    else:
                        return 14
                else:
                    print 'nops 2 3 error: invalid operand: ', op1
                    return 0

    strop = ' '
    op = 0
    op1 = ' '
    op2 = ' '
    nops = 0
    addr = 0
    size = 0
    w = 1
    mode = 0

    isdata = 0
    strdata = []
    data = []

def main():
    argx = re.compile(anyPattern, re.VERBOSE)
    irgx = re.compile(instrPattern, re.VERBOSE)
    drgx = re.compile(dirPattern, re.VERBOSE)

    b = 0
    il = [[],[],[],[],[],[]]
    romname = 'test.kpr'
    org = 0

    if(len(sys.argv) < 2):
        print 'no input files, exiting'
        exit(0)

    # Parse the input assembly source file
    f = open(sys.argv[1], "r")
    for line in f:
        result = argx.match(line)
        if result is not None:
            if result.group(1) is not None:
                #print 'Found label ', result.group(1)[:-1]
                defs[result.group(1)[:-1]] = 0

    f.seek(0)

    # First match all labels so symbol table is set up
    for line in f:
        # Handle instructions
        result = irgx.match(line)
        if result is not None:
            o = opcodes[result.group(2)]
            iii = None
            if o is None:
                print 'error: \'', result.group(2), '\' is not a valid opcode'
                continue
            if not result.group(5):
                if not result.group(4):
                    iii = instr(result.group(2), o, 0, org)
                else:
                    iii = instr(result.group(2), o, 1, result.group(4), org)
            else:
                iii = instr(result.group(2), o, 2, result.group(4), result.group(5), org)
            if result.group(1) is not None:
                defs[result.group(1)[:-1]] = org
            org += iii.size
            continue

        # Handle singleton labels
        result = argx.match(line)
        if result is not None:
            if result.group(1) is not None:
                defs[result.group(1)[:-1]] = org

    print 'Found following labels:'
    for ddd in defs:
        print '\t', ddd, ':', hex(defs[ddd])

    f.seek(0)
    org = 0
    for line in f:
        # Handle directives
        result = drgx.match(line)
        if result is not None:
            d = directives[result.group(1)]
            if d is None:
                print 'error: \'', result.group(1), ' \'is not a valid directive'
                continue
            # Bank directive
            if d == 0:
                b = banks[result.group(2)]
                org = bank_addrs[b]
                #print 'Setting bank to bank #', b, ' (', result.group(2), ')'
                continue
            # Data byte directive
            elif d == 1:
                db = instr(result.group(1), d, 0, org)
                db.isdata = 1
                db.strdata = list()
                for i in xrange(2,6):
                    if result.group(i) is not None:
                        db.strdata.append(result.group(i))
                db.size = len(db.strdata)
                il[b].append(db)
                org += db.size
            # Org (origin) directive
            elif d == 3:
                org = num(result.group(2))

            continue

        # Handle instructions
        result = irgx.match(line)
        if result is not None:
            o = opcodes[result.group(2)]
            iii = None
            if o is None:
                print 'error: \'', result.group(2), '\' is not a valid opcode'
                continue
            if not result.group(5):
                if not result.group(4):
                    iii = instr(result.group(2), o, 0, org)
                else:
                    iii = instr(result.group(2), o, 1, result.group(4), org)
            else:
                iii = instr(result.group(2), o, 2, result.group(4), result.group(5), org)
            if result.group(3) is not None:
                iii.w = 1 if result.group(3) == '.w' else 0
            il[b].append(iii)
            org += iii.size
            continue

    f.close()

    # Write to the output binary file (rom)
    magic = "KHPR"
    name = "Khepra test ROM\0"
    desc = "Simple ROM testing ops\0\0\0\0\0\0\0\0\0\0"

    f = open(romname, "wb")
    f.write(magic)
    f.write(struct.pack('I', 24652))
    f.write(struct.pack('I', 0))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('I', 0))
    f.write(name)
    f.write(desc)
    tc = 68     # Bytes written to file
    c = 0       # Bytes of ROM contents (excl. file header) written to file

    for bn in xrange(len(il)):
        if len(il[bn]) == 0:
            continue
        offs = bank_addrs[bn]    # Current offset in address space
        print 'Writing bank ', bn
        f.write(struct.pack('B',bn))
        f.write(struct.pack('B', 0))
        f.write(struct.pack('H',bank_sizes[bn]))
        for i in il[bn]:
            while offs < i.addr:
                f.write(struct.pack('B', 0))
                c += 1
                offs += 1
            # If this is a db/dw directive, write the data bytes
            if i.isdata:
                i.data = list()
                for d in i.strdata:
                    if d in defs:
                        i.data.append(defs[d])
                    else:
                        i.data.append(num(d))
                for db in i.data:
                    f.write(struct.pack('B', db))
                    c += 1
                    offs += 1
                continue
            # Otherwise process the instruction normally
            op1 = 0
            op2 = 0
            am = getAddrMode(i.nops, i.op1, i.op2, defs, i.addr+i.size)
            if am in [0,1,6,7,8,9,10,11,12]:
                if i.op1 in rr:
                    op1 = regs[i.op1]
                elif (len(i.op1) > 2 and i.op1[1] in rr):
                    op1 = regs[i.op1[1]]
            if am in [6,7,8,13,14]:
                if i.op2 in rr:
                    op2 = regs[i.op2]
                elif (len(i.op2) == 3 and i.op2[1] in rr):
                    op2 = regs[i.op2[1]]
            if am in [2,3,4,5,13,14] :
                opp = re.sub('[\[\]]', '', i.op1)
                if opp in defs:
                    op1 = defs[opp]
                else:
                    op1 = num(opp)
                if i.nops == 2:
                    op2 = regs[i.op2]
            elif am in [9,10,11,12] and type(i.op2) is str:
                op1 = regs[i.op1]
                opp = re.sub('[\[\]]', '', i.op2)
                if opp in defs:
                    op2 = defs[opp]
                else:
                    op2 = num(opp)
            b = (i.op << 3) | (i.w << 2) | (am >> 2)
            f.write(struct.pack('B', b))
            c += 1
            offs += 1
            if i.nops == 0:
                continue

            b = (am << 6) & 255
            if am in [0,1,6,7,8,9,10,11,12]:
                b |= (op1 << 3)
            if i.nops == 2 and am in [6,7,8,13,14]:
                b |= op2
            f.write(struct.pack('B', b))
            c += 1
            offs += 1

            if am in [2,3,4,5,13,14]:
                b = op1 & 0xff
                f.write(struct.pack('B', b))
                c += 1
                offs += 1
                if am in [4,5,14]:
                    b = op1 >> 8
                    f.write(struct.pack('B', b))
                    c += 1
                    offs += 1
            elif am in [9,10,11,12]:
                b = op2 & 0xff
                f.write(struct.pack('B', b))
                c += 1
                offs += 1
                if am in [11,12]:
                    b = op2 >> 8
                    f.write(struct.pack('B', b))
                    c += 1
                    offs += 1

        while c < bank_sizes[bn]:
            f.write(struct.pack('B', 0))
            c += 1
            offs += 1
        print 'Wrote', c, 'bytes to bank', bn
        tc += c
        c = 0
        bn += 1

    f.close()

    print 'Wrote', tc, 'bytes to', romname

if __name__ == "__main__":
    main()

