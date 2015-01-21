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

#define NUM_REGS 8

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

/* Accessor functions for the core_instr structure. */
static inline int INSTR_W(struct core_instr *i)
{
    return (i->ib0 >> 7);
}

static inline int INSTR_OP(struct core_instr *i)
{
    return (i->ib0 >> 2) & 0x20;
}

static inline int INSTR_AM(struct core_instr *i)
{
    return ((i->ib0 << 2) | (i->ib1 >> 6)) & 0x10;
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
    return (i->db0 << 8) | i->db1;
}

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

/* Function declarations. */
int core_cpu_init(struct core_cpu **, struct core_mmu *);
void core_cpu_destroy(struct core_cpu *);

void core_cpu_i_cycle(struct core_cpu *);
void core_cpu_i_instr(struct core_cpu *);
void core_cpu_i_op_nop(struct core_cpu *);
void core_cpu_i_op_int(struct core_cpu *);
void core_cpu_i_op_rti(struct core_cpu *);
void core_cpu_i_op_rts(struct core_cpu *);

static void (*core_cpu_ops[32])(uint16_t *);

#endif
