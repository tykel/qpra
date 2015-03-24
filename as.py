#!/bin/env python2

# as.py -- Assembler
#
# Simple Python script to convert Khepra assembly programs into
# working ROMs loadable by qpra and other emulators as per the specification.

import sys
import re
import struct

instrPattern = r"""
([a-zA-Z_-]+:)?         # Label, optional
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
(\$?\w+)
\s*
(\$?\w+)?
\s*
$
"""

directives = { ".bank":0, ".db":1, ".dw":2 }

banks = {
        "rom_fixed":0, "rom_swap":1, "tile_swap":2, "audio_swap":3
        }

opcodes = {
        "nop":0, "int":1, "rti":2, "rts":3, "jp":4, "cl":5, "jz":6, "cz":7,
        "jc":8, "cc":9, "jo":10, "co":11, "jn":12, "cn":13, "not":14, "inc":15,
        "dec":16, "ind":17, "ded":18, "mv":19, "cmp":20, "tst":21, "add":22,
        "sub":23, "mul":24, "div":25, "lsl":26, "lsr":27, "asr":28, "and":29,
        "or":30, "xor":31
        }

regs = { 'a':0, 'b':1, 'c':2, 'd':3, 'e':4, 'f':5, 'p':6, 's':7 }

def getAddrMode(nops, op1, op2, defs):
    #print 'getAddrMode: ', nops, ': ', op1, ', ', op2, ' -- ', defs
    if nops == 0:
        return 0
    if nops == 1:
        if re.match("[abcdef]", op1) is not None:
            return 0
        elif re.match("\[[abcdef]\]", op1) is not None:
            return 1
        elif re.match("(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)", op1) is not None:
            return 2
        elif re.match("\w+", op1) is not None and op1 in defs:
            return 2
        elif re.match("\[(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)\]", op1) is not None:
            return 3
        elif re.match("\[\d+\]", op1) is not None and op1 in defs:
            return 3
        elif re.match("(\$[abcdef0-9]{3,4}?)|(\d{3,5}?)", op1) is not None:
            return 4
        elif re.match("\w+", op1) is not None and op1 in defs:
            return 4
        elif re.match("\[(\$[abcdef0-9]{3,4}?)|(\d{3,5}?)\]", op1) is not None:
            return 5
        elif re.match("\[\w+\]", op1) is not None and op1 in defs:
            return 5
        else:
            print 'error: invalid operand: ', op1
            return 0
    elif nops == 2:
        if re.match("[abcdef]", op1) is not None:
            if re.match("[abcdef]", op2) is not None:
               return 6
            elif re.match("\[[abcdef]\]", op2) is not None:
                return 7
            elif re.match("(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)", op2) is not None:
                return 9
            elif re.match("\w+", op2) is not None and op2 in defs:
                return 9
            elif re.match("\[(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)\]", op2) is not None:
                return 10
            elif re.match("\[\w+\]", op2) is not None and op2 in defs:
                return 10
            elif re.match("(\$[abcdef0-9]{3,4}?)|(\d{3,5}?)", op2) is not None:
                return 11
            elif re.match("\w+", op2) is not None and op2 in defs:
                return 11
            elif re.match("\[(\$[abcdef0-9]{3,4}?)|(\d{3,5}?)\]", op2) is not None:
                return 12
            elif re.match("\[\w+\]", op2) is not None and op2 in defs:
                return 12
            else:
                print 'error: invalid operand: ', op2
                return 0
        elif re.match("\[[abcdef]\]", op1) is not None:
            if re.match("[abcdef]", op2) is not None:
                return 8
            else:
                print 'error: invalid operand: ', op2
                return 0
        elif re.match("[abcdef]", op2) is not None:
            if re.match("\[(\$[abcdef0-9]{1,2}?)|(\d{1,3}?)\]", op1) is not None:
                return 13
            elif re.match("\[\w+\]", op1) is not None and op1 in defs:
                return 13
            elif re.match("\[(\$[abcdef0-9]{3,4}?)|(\d{3,5}?)\]", op1) is not None:
                return 14
            elif re.match("\[\w+\]", op1) is not None and op1 in defs:
                return 14
            else:
                print 'error: invalid operand: ', op1
                return 0



def num(s):
    try:
        return int(s)
    except ValueError:
        return float(s)


class instr:
    def __init__(self, strop, op, nops, op1="", op2=""):
        self.strop = strop
        self.op = op
        self.nops = nops
        if nops > 0:
            self.op1 = op1
        if nops > 1:
            self.op2 = op2

    strop = ' '
    op = 0
    op1 = ' '
    op2 = ' '
    nops = 0

def main():
    irgx = re.compile(instrPattern, re.VERBOSE)
    drgx = re.compile(dirPattern, re.VERBOSE)

    b = 0
    il = [[],[],[],[],[],[]]
    romname = 'test.kpr'
    defs = { 'loop':0xc, 'test':0x3, 'init':0x0}

    if(len(sys.argv) < 2):
        print 'no input files, exiting'
        exit(0)

    # Parse the input assembly source file
    f = open(sys.argv[1], "r")
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
                #print 'Setting bank to bank #', b, ' (', result.group(2), ')'
                continue

            continue

        # Handle instructions
        result = irgx.match(line)
        if result is not None:
            o = opcodes[result.group(2)]
            if o is None:
                print 'error: \'', result.group(2), '\' is not a valid opcode'
                continue
            if not result.group(5):
                if not result.group(4):
                    il[b].append(instr(result.group(2), o, 0))
                else:
                    il[b].append(instr(result.group(2), o, 1, result.group(4)))
            else:
                il[b].append(instr(result.group(2), o, 2, result.group(4), result.group(5)))
            #print 'Emitting op', hex(o), '(', result.group(2), ')'
            continue

    f.close()

    # Write to the output binary file (rom)
    magic = "KHPR"
    name = "Khepra test ROM\0"
    desc = "Simple ROM testing ops\0\0\0\0\0\0\0\0\0\0"

    f = open(romname, "wb")
    f.write(magic)
    f.write(struct.pack('I', 16*1024))
    f.write(struct.pack('I', 0))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('B', 1))
    f.write(struct.pack('I', 0))
    f.write(name)
    f.write(desc)
    c = 68
    for b in il:
        for i in b:
            op1 = 0
            op2 = 0
            am = getAddrMode(i.nops, i.op1, i.op2, defs)
            if am in [2,3,4,5,13,14] :
                if i.op1 in defs:
                    op1 = defs[i.op1]
                else:
                    op1 = num(i.op1)
                if i.nops == 2:
                    op2 = regs[i.op2]
            elif am in [9,10,11,12] and type(i.op2) is str:
                op1 = regs[i.op1]
                if i.op2 in defs:
                    op2 = defs[i.op2]
                else:
                    op2 = num(i.op2)
            #print 'am = ', am, ', op1 = ', op1, ', op2 = ', op2
            b = (i.op << 3) | (am >> 2)
            #print 'output b = ', hex(b)
            f.write(struct.pack('B', b))
            c += 1
            if i.nops == 0:
                continue

            b = (am << 6) & 255
            #print 'interm b = ', hex(b)
            if am in [0,1,6,7,8,9,10,11,12]:
                b |= (op1 << 3)
            if i.nops == 2 and am in [6,7,8,13,14]:
                b |= op2
            #print 'output b = ', hex(b)
            f.write(struct.pack('B', b))
            c += 1

            if am in [2,3,4,5,13,14]:
                b = op1 & 0xff
                f.write(struct.pack('B', b))
                c += 1
                if am in [4,5,14]:
                    b = op1 >> 8
                    #print 'output b = ', hex(b)
                    f.write(struct.pack('B', b))
                    c += 1
            elif am in [9,10,11,12]:
                b = op2 & 0xff
                f.write(struct.pack('B', b))
                c += 1
                if am in [11,12]:
                    b = op2 >> 8
                    #print 'output b = ', hex(b)
                    f.write(struct.pack('B', b))
                    c += 1
    while c-68 < 16*1024:
        f.write(struct.pack('B', 0))
        c += 1

    f.close()

    print 'Wrote', c, 'bytes to', romname

if __name__ == "__main__":
    main()

