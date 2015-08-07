#ifndef __cpu_h__
#define __cpu_h__

#include <stdint.h>

#include "common.h"

typedef enum {
    STAGE_FD = 1, STAGE_FE = 2, STAGE_FP = 4, STAGE_EX = 8, STAGE_ST = 16,
    __STAGE_MAX = 32
} e_stage;

struct opdata {
    uint16_t s;
    uint16_t o;
    uint16_t *d;
    uint16_t *f;
    uint16_t ff;
    uint16_t *p;
};

typedef void (*opptr_t)(struct opdata *);

struct cpu_state {
#pragma pack(push, 1)
    union {
        uint16_t r[8];
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t x;
            uint16_t y;
            uint16_t p;
            uint16_t s;
            uint16_t f;
        };
    };

    uint8_t ib0;
    uint8_t ib1;
    uint8_t ib2;
    uint8_t ib3;
#pragma pack(pop)

    uint16_t latch_a;
    uint16_t latch_in;
    uint16_t latch_out;
    bool irq_pending;

    e_stage stage;
    int istages;

    int cycle;
    int icycle;
    int icycles;

    uint8_t m[64 * 1024];

    opptr_t op[16];
    
    uint16_t p_old;
};

bool cpu_init(struct cpu_state *s);
bool cpu_cycle(struct cpu_state *s);
bool cpu_destroy(struct cpu_state *s);

void cpu_op_nop();
void cpu_op_add(struct opdata *);
void cpu_op_sub(struct opdata *);
void cpu_op_mul(struct opdata *);
void cpu_op_div(struct opdata *);
void cpu_op_lsl(struct opdata *);
void cpu_op_lsr(struct opdata *);
void cpu_op_asr(struct opdata *);
void cpu_op_asr(struct opdata *);
void cpu_op_and(struct opdata *);
void cpu_op_or(struct opdata *);
void cpu_op_xor(struct opdata *);
void cpu_op_not(struct opdata *);
void cpu_op_mvr(struct opdata *);
void cpu_op_jpx(struct opdata *);
void cpu_op_jp(struct opdata *);
void cpu_op_mvm(struct opdata *);

#endif
