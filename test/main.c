#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

int main(int argc, char *argv)
{
    int i;
    struct cpu_state s;

    cpu_init(&s);

    uint8_t program[] = {
        0xc8, 0x00, 0x04, // MVR 4, a
        0xc9, 0x00, 0x00, // MVR 0, b
        0x19, 0x10, 0x01, //-ADD b, 1, b
        0x28, 0x00, 0x01, // SUB a, 1, a
        0xd8, 0x02,       // JPz +
        0xe8, 0xf6,       // JP  -
        0xe8, 0xfe,       //+JP  +
    };
    memcpy(s.m, program, sizeof(program));

    for(i = 0; i < 60; i++) {
        printf("\nAfter %d cycle%s: p = %04x; a = %04x, b = %04x\n",
               i, i!=1?"s":" ", s.p, s.a, s.b);
        cpu_cycle(&s);
    }
    printf("\nAfter %d cycles: p = %04x; a = %04x\n, b = %04x",
           i, s.p, s.a, s.b);

    cpu_destroy(&s);

    return 0;
}

// TODO:
//      - get a proper reg to reg instruction in there
//      - get flags working
//      - write a debugging interface