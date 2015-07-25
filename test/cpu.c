/*
 * Khepra CPU core v2 (Experimental)
 */

#include <stdio.h>

#include "cpu.h"


static inline const char* stagestr(e_stage stage) {
    switch(stage) {
        case STAGE_FD: return "STAGE_FD <Fetch/Decode>";
        case STAGE_FE: return "STAGE_FE <Fetch Extra b>";
        case STAGE_FP: return "STAGE_FP <Fetch Ptr data>";
        case STAGE_EX: return "STAGE_EX <EXecute>";
        case STAGE_ST: return "STAGE_ST <STore>";
        default: return "<unknown stage>";
    }
}

#define FLAG_I (1 << 4)
#define FLAG_N (1 << 3)
#define FLAG_O (1 << 2)
#define FLAG_C (1 << 1)
#define FLAG_Z (1 << 0)

static inline int flag_n(struct cpu_state *s) { return !!(s->f & FLAG_N); }
static inline void setf_n(uint16_t *f, bool e)
{
    *f &= ~FLAG_N;
    *f |= e ? FLAG_N : 0;
}
static inline int flag_o(struct cpu_state *s) { return !!(s->f & FLAG_O); }
static inline void setf_o(uint16_t *f, bool e)
{
    *f &= ~FLAG_O;
    *f |= e ? FLAG_O : 0;
}
static inline int flag_c(struct cpu_state *s) { return !!(s->f & FLAG_C); }
static inline void setf_c(uint16_t *f, bool e)
{
    *f &= ~FLAG_C;
    *f |= e ? FLAG_C : 0;
}
static inline int flag_z(struct cpu_state *s) { return !!(s->f & FLAG_Z); }
static inline void setf_z(uint16_t *f, bool e)
{
    *f &= ~FLAG_Z;
    *f |= e ? FLAG_Z : 0;
}

#define MODE_RRR    (0 | (0 << 2))
#define MODE_RIR    (1 | (0 << 2))
#define MODE_RR     (0 | (1 << 2))
#define MODE_IR     (1 | (1 << 2))
#define MODE_p      (0 | (2 << 2))
#define MODE_P      (1 | (2 << 2))
#define MODE_PR     (0 | (4 << 2))
#define MODE_pR     (1 | (4 << 2))
#define MODE_RP     (2 | (4 << 2))
#define MODE_Rp     (3 | (4 << 2))

#define BIT_MODE (1 << 3)

static inline int op(struct cpu_state *s) { return (s->ib0 >> 4); }

static inline const char* opstr(int op) {
    switch(op) {
        case 0: return  "nop";
        case 1: return  "add s, o, d";
        case 2: return  "sub s, o, d";
        case 3: return  "mul s, o, d";
        case 4: return  "div s, o, d";
        case 5: return  "lsl s, o, d";
        case 6: return  "lsr s, o, d";
        case 7: return  "asr s, o, d";
        case 8: return  "or s, o, d";
        case 9: return  "xor s, o, d";
        case 10: return "and s, o, d";
        case 11: return "not s, d";
        case 12: return "mvr s, d";
        case 13: return "jpx d";
        case 14: return "jp  d";
        case 15: return "mvm s, d";
    }
}

static inline int mode(struct cpu_state *s)
{
    int o = op(s);
    int tag = o < 11 ? (0 << 2) : o < 13 ? (1 << 2) : o < 15 ? (2 << 2) : (4 << 2);
    return (op(s) == 15) ? ((s->ib0 >> 2) & 3) | tag : (!!(s->ib0 & BIT_MODE) | tag);
}

static inline const char* modestr(int mode) {
    switch(mode) {
        case MODE_RRR: return "MODE_RRR <rs,   ro,  rd>";
        case MODE_RIR: return "MODE_RIR <rs,   imm, rd>";
        case MODE_RR: return  "MODE_RR  <rs,   rd     >";
        case MODE_IR: return  "MODE_IR  <imm,  rd     >";
        case MODE_p: return   "MODE_p   <[rs]         >";
        case MODE_P: return   "MODE_P   <[imm]        >";
        case MODE_PR: return  "MODE_PR  <[imm],rd     >";
        case MODE_pR: return  "MODE_pR  <[rs], rd     >";
        case MODE_RP: return  "MODE_RP  <rd,   [imm]  >";
        case MODE_Rp: return  "MODE_Rp  <rs,   [rd]   >";
        default: return "<unknown mode>";
    }
}

#define MASK_SRC_0  (7 << 0)
#define MASK_SRC_1  (7 << 4)
#define SHFT_SRC_1  4

static inline int src(struct cpu_state *s)
{
    switch(mode(s)) {
        case MODE_RRR:
        case MODE_RIR:
        case MODE_RR:
            return s->ib0 & MASK_SRC_0;
        case MODE_p:
        case MODE_pR:
        case MODE_RP:
        case MODE_Rp:
            return (s->ib1 & MASK_SRC_1) >> SHFT_SRC_1;
    }
}

#define MASK_OPD    (7 << 4)
#define SHFT_OPD    4

static inline int opd(struct cpu_state *s)
{
    return (s->ib1 & MASK_OPD) >> SHFT_OPD;
}

#define MASK_DST_0  (7 << 0)
#define MASK_DST_1  (7 << 4)
#define SHFT_DST_1  4

static inline int dst(struct cpu_state *s)
{
    switch(mode(s)) {
        case MODE_RRR:
        case MODE_Rp:
            return s->ib1 & MASK_DST_0;
        case MODE_RIR:
        case MODE_RR:
        case MODE_PR:
            return (s->ib1 & MASK_DST_1) >> SHFT_DST_1;
        case MODE_IR:
            return s->ib0 & MASK_DST_0;
    }
}

static inline uint16_t imm(struct cpu_state *s)
{
    switch(mode(s)) {
        case MODE_RIR:
        case MODE_IR:
        case MODE_PR:
        case MODE_RP:
            return (s->ib1 & 1) ? *(uint16_t *)&s->ib2 : *(uint8_t *)&s->ib2;
        case MODE_P:
            return (s->ib0 & 1) ? *(uint16_t *)&s->ib1 : *(uint8_t *)&s->ib1;
    }
}

#define MASK_FF (3 << 1)
#define SHFT_FF 1

static inline int ff(struct cpu_state *s)
{
    return 1 << ((s->ib0 & MASK_FF) >> SHFT_FF);
}

static inline int stages(struct cpu_state *s)
{
    if(op(s) == 0) return STAGE_FD | STAGE_EX;
    switch(mode(s)) {
        case MODE_RRR:      return STAGE_FD | STAGE_EX;
        case MODE_RIR:      return STAGE_FD | STAGE_FE | STAGE_EX;
        case MODE_RR:       return STAGE_FD | STAGE_EX;
        case MODE_IR:       return STAGE_FD | STAGE_FE | STAGE_EX;
        case MODE_p:        return STAGE_FD | STAGE_FP | STAGE_EX;
        case MODE_P:
            if(s->ib0 & 1)  return STAGE_FD | STAGE_FE | STAGE_EX;
            else            return STAGE_FD | STAGE_EX;
        case MODE_PR:       return STAGE_FD | STAGE_FE | STAGE_FP | STAGE_EX;
        case MODE_pR:       return STAGE_FD | STAGE_FP | STAGE_EX;
        case MODE_RP:       return STAGE_FD | STAGE_FE | STAGE_EX | STAGE_ST;
        case MODE_Rp:       return STAGE_FD | STAGE_EX | STAGE_ST;
    }
}

static inline int cycles(struct cpu_state *s)
{
    int o = op(s);
    int m = mode(s);

    // NOP
    if(o == 0) return 2;
    // ADD...AND
    else if(o < 11) return 2 + (m == MODE_RIR);
    // NOT, MVR
    else if(o < 13) return 2 + (m == MODE_IR);
    // JPx, JP
    else if(o < 15) return 2 + (m == MODE_p) + (m == MODE_P && (s->ib0 & 1));
    // MVM
    else if(o < 16) return 3 + (m == MODE_PR || m == MODE_RP);

    // Should not get here!
    return -1;
}

static inline uint16_t readpc(struct cpu_state *s) {
    uint16_t pc = *(uint16_t *)&s->m[s->p];
    s->p += 2;
    return pc;
}

// Fetch/Decode cycle
bool cpu_cycle_fd(struct cpu_state *s)
{
    *(uint16_t *)&s->ib0 = readpc(s);
    if(mode(s) == MODE_P && s->ib0 & 1) s->p -= 1;
    if(mode(s) == MODE_pR) s->latch_a = s->r[src(s)];
    return true;
}

// Fetch Extra instruction data cycle
bool cpu_cycle_fe(struct cpu_state *s)
{
    if(mode(s) == MODE_P) {
        *(uint16_t *)&s->ib1 = readpc(s);
        if(~s->ib0 & 1) s->p -= 1;
    } else {
        *(uint16_t *)&s->ib2 = readpc(s);
        if(~s->ib1 & 1) s->p -= 1;
    }

    if(mode(s) == MODE_P || mode(s) == MODE_PR) {
        s->latch_a = imm(s);
        // Add the current PC to address latch if relative jump (1 byte offset)
        if(mode(s) == MODE_P) {
            s->latch_a = (s->ib0 & 1) ? s->latch_a : *(int8_t *)&s->latch_a + s->p;
        } else {
            s->latch_a = (s->ib1 & 1) ? s->latch_a : *(int8_t *)&s->latch_a + s->p;
        }
    }

    return true;
}

// Fetch Pointed data cycle
bool cpu_cycle_fp(struct cpu_state *s)
{
    s->latch_in = s->m[s->latch_a];
    return true;
}

// EXecute cycle
bool cpu_cycle_ex(struct cpu_state *s)
{
    int o = op(s);
    int m = mode(s);
    struct opdata x;

    // NOP
    if(!o) {
        cpu_op_nop(&x);
        return true;
    }
    switch(m) {
        case MODE_RRR:
            x.s = s->r[src(s)];
            x.o = s->r[opd(s)];
            x.d = &s->r[dst(s)];
            x.f = &s->f;
            s->op[o](&x);
            break;
        case MODE_RIR:
            x.s = s->r[src(s)];
            x.o = imm(s);
            x.d = &s->r[dst(s)];
            x.f = &s->f;
            s->op[o](&x);
            break;
        case MODE_RR:
            x.s = s->r[src(s)];
            x.d = &s->r[dst(s)];
            x.f = &s->f;
            s->op[o](&x);
            break;
        case MODE_IR:
            x.s = imm(s);
            x.d = &s->r[dst(s)];
            x.f = &s->f;
            s->op[o](&x);
            break;
        case MODE_p:
        case MODE_P:
            if(m == MODE_P) {
                int i = imm(s);
                x.s = (s->ib0 & 1) ? i : *(int8_t *)&i + s->p;
            } else {
                x.s = s->latch_in;
            }
            x.f = &s->f;
            x.ff = ff(s);
            x.p = &s->p;
            s->op[o](&x);
            break;
        case MODE_PR:
        case MODE_pR:
            x.s = s->latch_in;
            x.d =&s->r[dst(s)];
            x.f = &s->f;
            s->op[o](&x);
            break;
        case MODE_RP:
        case MODE_Rp:
            if(m == MODE_RP) {
                // If the W bit is not set, treat imm as 8-bit 2's comp. offset
                int i = imm(s);
                s->latch_a = (s->ib1 & 1) ? i : (s->p + *(int8_t *)&i);
            } else {
                s->latch_a = s->r[dst(s)];
            }
            x.s = s->r[src(s)];
            x.d = &s->latch_out;
            x.f = &s->f;
            s->op[o](&x);
            break;
    }
    return true;
}

// STore cycle
bool cpu_cycle_st(struct cpu_state *s)
{
    *(uint16_t *)&s->m[s->latch_a] = s->latch_out;
    return true;
}

// Execute one cycle in the processor
bool cpu_cycle(struct cpu_state *s)
{
    printf("stage: %s\n", stagestr(s->stage));
    switch(s->stage) {
        case STAGE_FD:
            s->ib0 = s->ib1 = s->ib2 = s->ib3 = 0;
            cpu_cycle_fd(s);
            // Determine all the stages of this instruction
            s->istages = stages(s);
            s->icycles = cycles(s);
            s->cycle = 1;
            printf("--- found op   %s\n", opstr(op(s)));
            printf("--- found mode (%03x) %s\n", mode(s), modestr(mode(s)));
            break;
        case STAGE_FE:
            cpu_cycle_fe(s);
            break;
        case STAGE_FP:
            cpu_cycle_fp(s);
            break;
        case STAGE_EX:
            cpu_cycle_ex(s);
            break;
        case STAGE_ST:
            cpu_cycle_st(s);
            break;
    }

    // Find the next stage in the instruction
    do s->stage <<= 1; while((s->istages & s->stage) == 0 && s->stage < __STAGE_MAX);
    // If we hit the end, loop around
    if(s->stage == __STAGE_MAX) {
        s->stage = STAGE_FD;
        printf("----------------------------------------\n");
    }

    return true;
}

bool cpu_init(struct cpu_state *s)
{
    s->stage = STAGE_FD;

    s->op[0] = cpu_op_nop;
    s->op[1] = cpu_op_add;
    s->op[2] = cpu_op_sub;
    s->op[3] = cpu_op_mul;
    s->op[4] = cpu_op_div;
    s->op[5] = cpu_op_lsl;
    s->op[6] = cpu_op_lsr;
    s->op[7] = cpu_op_asr;
    s->op[8] = cpu_op_or;
    s->op[9] = cpu_op_xor;
    s->op[10] = cpu_op_and;
    s->op[11] = cpu_op_not;
    s->op[12] = cpu_op_mvr;
    s->op[13] = cpu_op_jpx;
    s->op[14] = cpu_op_jp;
    s->op[15] = cpu_op_mvm;
}

bool cpu_destroy(struct cpu_state *s)
{
    return true;
}

// Opcodes (interpreted)

// NOP
void cpu_op_nop(struct opdata *x) {}

// ADD
void cpu_op_add(struct opdata *x)
{
    *x->d = x->s + x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, (x->s + x->o) & ~0xffff);
    setf_o(x->f, (*x->d & 0x8000) ^ (x->s & 0x8000));
    //printf("add %d, %d = %d\n", x->s, x->o, *x->d);
}

// SUB
void cpu_op_sub(struct opdata *x)
{
    *x->d = x->s - x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, (x->s - x->o) & ~0xffff);
    setf_o(x->f, (*x->d & 0x8000) ^ (x->s & 0x8000));
    //printf("sub %d, %d = %d\n", x->s, x->o, *x->d);
}

// MUL
void cpu_op_mul(struct opdata *x)
{
    *x->d = x->s * x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, (x->s * x->o) & ~0xffff);
    setf_o(x->f, (*x->d & 0x8000) ^ (x->s & 0x8000));
}

// DIV
void cpu_op_div(struct opdata *x)
{
    *x->d = x->s / x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, (x->s / x->o) & ~0xffff);
    setf_o(x->f, (*x->d & 0x8000) ^ (x->s & 0x8000));
}

// LSL
void cpu_op_lsl(struct opdata *x)
{
    *x->d = x->s << x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, x->s & (1 << (16 - x->o)));
}

// LSR
void cpu_op_lsr(struct opdata *x)
{
    *x->d = x->s >> x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, x->s & (1 << (x->o - 1)));
}

// ASR
void cpu_op_asr(struct opdata *x)
{
    *(int16_t *)x->d = (int16_t)x->s >> x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
    setf_c(x->f, x->s & (1 << (x->o - 1)));
}

// OR
void cpu_op_or(struct opdata *x)
{
    *x->d = x->s | x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
}

// XOR
void cpu_op_xor(struct opdata *x)
{
    *x->d = x->s ^ x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
}

// AND
void cpu_op_and(struct opdata *x)
{
    *x->d = x->s & x->o;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
}

// NOT
void cpu_op_not(struct opdata *x)
{
    *x->d = ~x->s;
    setf_z(x->f, *x->d == 0);
    setf_n(x->f, *x->d & 0x8000);
}

// MVR
void cpu_op_mvr(struct opdata *x)
{
    *x->f = *x->f & 0xf0;
    *x->d = x->s;
}

// JPx
void cpu_op_jpx(struct opdata *x)
{
    *x->p = (*x->f & x->ff) ? x->s : *x->p;
}

// JP
void cpu_op_jp(struct opdata *x)
{
    *x->p = x->s;
}

// MV
void cpu_op_mvm(struct opdata *x)
{
    *x->f = *x->f & 0xf0;
    *x->d = x->s;
}
