Khepra CPU
==========

Model KPR001, "SO16-MP"

16-bit
8 registers
64 KiB physical address space

regs: 8 = 3 bits
====

A, B, C, X, Y, P, S, F

internal:
--------
mem addr-latch (@) (addr to read/write)
mem out-latch (>) (value to write), mem in-latch (<) (read result)
IRQ bit

flags:
-----
000I NOCZ

ops: 14 = 4 bits
===

0   NOP                 ___
1   ADD S1, S2, D       D = S1 + S2
2   SUB S1, S2, D       D = S1 - S2
3   MUL S1, S2, D       D = S1 * S2
4   DIV S1, S2, D       D = S1 / S2
5   LSL S1, S2, D       D = S1 << S2
6   LSR S1, S2, D       D = S1 >> S2
7   ASR S1, S2, D       D = S1 >> S2    [D sign-extended]
8   OR  S1, S2, D       D = S1 | S2
9   XOR S1, S2, D       D = S1 ^ S2
a   AND S1, S2, D       D = S1 & S2
b   NOT S1, D           D = ~S1
c   MVR S1, D           D = S1
d   Jx  D               P = D iff F.x
e   JP D                P = D
f   MVM S1, D           D = S1

Modes: 2 = 1 bit OR 4 = 2 bits
=====

OP R, R, R      RRR     0
OP R, Imm, R    RIR     1

OP R, R         RR      0
OP Imm, R       IR      1

OP [R]          p       0
OP [Ptr]        P       1

OP [Ptr], R     PR      0
OP [R], R       pR      1
OP R, [Ptr]     RP      2
OP R, [R]       Rp      3

Instruction format:
==================
W = Word bit (read full 16-bit/2-byte immediate, instead of 8-bit/1-byte)

RRR
oooo 0aaa  _bbb _ddd

RIR
oooo 1aaa  _ddd ___W  IIII IIII [IIII IIII]

RR
oooo 0aaa  _ddd ____

IR
oooo 1ddd  ____ ___W  IIII IIII [IIII IIII]

p
oooo 0ff_  _aaa ____

P
oooo 1ffW  IIII IIII [IIII IIII]

PR
oooo 00__  _ddd ___W  IIII IIII [IIII IIII]

pR
oooo 01__  _aaa _ddd

RP
oooo 10__  _aaa ___W  IIII IIII [IIII IIII]

Rp
oooo 11__  _aaa _ddd

Instruction cycles:
==================
___ : 2 = 1 + 1             [fetch/dec, exec]                       Used by NOP, no addressing mode used
RRR : 2 = 1 + 1             [fetch/dec, exec]
RIR : 3 = 1 + 1 + 1         [fetch/dec, fetch, exec]
RR  : 2 = 1 + 1             [fetch/dec, exec]
IR  : 3 = 1 + 1 + 1         [fetch/dec, fetch, exec]
p   : 3 = 1 + 1 + 1         [fetch/dec, fetch, exec]
P   :2/3= 1 (+ 1) + 1       [fetch/dec, (fetch), exec]              Fetch #2 (Imm) not needed iff W==0
PR  : 4 = 1 + 1 + 1 + 1     [fetch/dec, fetch, fetch, exec]
pR  : 3 = 1 + 1 + 1         [fetch/dec, fetch, exec]
RP  : 4 = 1 + 1 + 1 + 1     [fetch/dec, fetch, exec, store]
Rp  : 3 = 1 + 1 + 1         [fetch/dec, exec, store]
