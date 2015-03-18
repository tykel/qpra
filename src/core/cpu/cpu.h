/*
 * core/cpu/cpu.h -- Emulator CPU functions (header).
 *
 * Defines the structures used to represent the CPU and related things.
 * Declares all the CPU functions including instructions.
 *
 */

#ifndef QPRA_CORE_CPU_H
#define QPRA_CORE_CPU_H

#include <stdint.h>

#define NUM_INSTRS 32
#define NUM_REGS 8

#define FLAG_Z  0x01
#define FLAG_C  0x02
#define FLAG_O  0x04
#define FLAG_N  0x08
#define FLAG_I  0x10

/* Instruction structure. */
struct core_instr
{
    uint8_t ib0;
    uint8_t ib1;
    uint8_t db0;
    uint8_t db1;
};

struct core_mmu;

/* CPU state structure. */
struct core_cpu
{
    /* Register file. Indexed using enum core_rid. */
    uint16_t r[NUM_REGS];
    /* Pointer to address space manager. */
    struct core_mmu *mmu;

    /* Pointer to current instruction. */
    struct core_instr *i;
    /* Instruction timer; how many cycles the current instruction has used. */
    int i_cycles;
    /* */
};

/* Enum for symbolic register file access. */
enum core_rid
{
    R_A, R_B, R_C, R_D, R_E,
    R_P,
    R_S,
    R_F,
    R_INVALID
};

/* Enum for operand size. */
enum core_opsz
{
    OP_16, OP_8
};

/* Struct for passing parameters to instructions. */
struct core_instr_params
{
    uint16_t op1;
    uint16_t op2;

    enum core_opsz size;
    int start_cycle;

    /* Save registers which may be overwritten */
    uint16_t p;
    uint16_t s;
    uint16_t f;
};

enum core_instr_name {
    OP_NOP, OP_INT, OP_RTI, OP_RTS, OP_JP, OP_CL, OP_JZ, OP_CZ, OP_JC, OP_CC,
    OP_JO, OP_CO, OP_JN, OP_CN, OP_NOT, OP_INC, OP_DEC, OP_IND, OP_DED, OP_MV,
    OP_CMP, OP_TST, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LSL, OP_LSR, OP_ASR,
    OP_AND, OP_OR, OP_XOR
};

enum core_mode_name {
    AM_DR, AM_IR, AM_DB, AM_IB, AM_DW, AM_IW, AM_DR_DR, AM_DR_IR, AM_IR_DR,
    AM_DR_DB, AM_DR_IB, AM_DR_DW, AM_DR_IW, AM_IB_DR, AM_IW_DR, AM_RESERVED
};

/* Accessor functions for the core_instr structure. */
static inline int INSTR_W(struct core_instr *i)
{
    return (i->ib0 >> 2) & 0x1;
}

static inline int INSTR_OP(struct core_instr *i)
{
    return i->ib0 >> 3;
}

static inline int INSTR_AM(struct core_instr *i)
{
    return ((i->ib0 << 2) | (i->ib1 >> 6)) & 0x0f;
}

static inline int INSTR_RX(struct core_instr *i)
{
    return (i->ib1 >> 3) & 0x8;
}

static inline int INSTR_RY(struct core_instr *i)
{
    return i->ib1 & 0x8;
}

static inline uint8_t INSTR_D8(struct core_instr *i)
{
    return i->db0;
}

static inline uint16_t INSTR_D16(struct core_instr *i)
{
    return *(uint16_t *)&i->db0;
}

static inline enum core_opsz INSTR_OPSZ(struct core_instr *i)
{
    return (i->ib0 & 0x80) ? OP_16 : OP_8;
}

/* Utility functions for querying various parameters about instructions. */
static inline int instr_is_void(struct core_instr *i)
{
    return INSTR_OP(i) < 4;
}

static inline int instr_is_1op(struct core_instr *i)
{
    return INSTR_AM(i) < AM_DR_DR;
}

static inline int instr_is_2op(struct core_instr *i)
{
    return INSTR_AM(i) >= AM_DR_DR;
}

static inline int instr_is_op1ptr(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_IR || am == AM_IB || am == AM_IW || am == AM_IR_DR ||
            am == AM_IB_DR || am == AM_IW_DR);
}

static inline int instr_is_op2ptr(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DR_IR || am == AM_DR_IB || am == AM_DR_IW);
}

static inline int instr_is_op1data(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DB || am == AM_DW);
}

static inline int instr_is_op2data(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DR_DB || am == AM_DR_DW);
}

static inline int instr_is_srcptr(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_IR || am == AM_IB || am == AM_IW || am == AM_DR_IR ||
            am == AM_DR_IB || am == AM_DR_IW);
}

static inline int instr_is_dstptr(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_IR || am == AM_IB || am == AM_IW || am == AM_IR_DR ||
            am == AM_IB_DR || am == AM_IW_DR);
}

static inline int instr_has_ptr(struct core_instr *i)
{
    return (instr_is_op1ptr(i) || instr_is_op2ptr(i));
}

static inline int instr_has_db(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DB || am == AM_IB || am == AM_DR_DB || am == AM_DR_IB ||
            am == AM_IB_DR);
}

static inline int instr_has_dw(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DW || am == AM_IW || am == AM_DR_DW || am == AM_DR_IW ||
            am == AM_IW_DR);
}

static inline int instr_has_data(struct core_instr *i)
{
    return (instr_has_db(i) || instr_has_dw(i));
}

static inline int instr_dr_only(struct core_instr *i)
{
    int am = INSTR_AM(i);
    return (am == AM_DR || am == AM_DR_DR);
}

static inline int instr_has_spderef(struct core_instr *i)
{
    int op = INSTR_OP(i);
    return (op == OP_INT || op == OP_RTI || op == OP_RTS || op == OP_CL
            || op == OP_CZ || op == OP_CC || op == OP_CO || op == OP_CN);
}


/* Function declarations. */
int core_cpu_init(struct core_cpu **, struct core_mmu *);
void core_cpu_destroy(struct core_cpu *);

void core_cpu_i_cycle(struct core_cpu *);
void core_cpu_i_instr(struct core_cpu *);
void core_cpu_i_op_nop(struct core_cpu *);
void core_cpu_i_op_int(struct core_cpu *);
void core_cpu_i_op_rti(struct core_cpu *);
void core_cpu_i_op_rts(struct core_cpu *);
void core_cpu_i_op_jp(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_cl(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_jz(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_cz(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_jc(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_cc(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_jo(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_co(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_jn(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_cn(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_not(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_inc(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_dec(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_ind(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_ded(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_mv(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_cmp(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_tst(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_add(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_sub(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_mul(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_div(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_lsl(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_lsr(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_asr(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_and(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_or(struct core_cpu *, struct core_instr_params *);
void core_cpu_i_op_xor(struct core_cpu *, struct core_instr_params *);

static void (*core_cpu_ops[32])(struct core_cpu *, struct core_instr_params *);

#endif
