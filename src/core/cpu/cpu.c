/*
 * core/cpu/cpu.c -- Emulator CPU functions.
 *
 * The CPU functions, notably the instructions, are implemented here.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "core/cpu/cpu.h"
#include "core/mmu/mmu.h"
#include "log.h"

static char *instrnam[NUM_INSTRS] = {
    "nop", /* 00 */
    "int", /* 01 */
    "rts", /* 02 */
    "rti", /* 03 */
    "jp",  /* 04 */
    "cl",  /* 05 */
    "jz",  /* 06 */
    "cz",  /* 07 */
    "jc",  /* 08 */
    "cc",  /* 09 */
    "jo",  /* 0a */
    "co",  /* 0b */
    "jn",  /* 0c */
    "cn",  /* 0d */
    "not", /* 0e */
    "inc", /* 0f */
    "dec", /* 10 */
    "ind", /* 11 */
    "ded", /* 12 */
    "mv",  /* 13 */
    "cmp", /* 14 */
    "tst", /* 15 */
    "add", /* 16 */
    "sub", /* 17 */
    "mul", /* 18 */
    "div", /* 19 */
    "lsl", /* 1a */
    "lsr", /* 1b */
    "asr", /* 1c */
    "and", /* 1d */
    "or",  /* 1e */
    "xor"  /* 1f */
};

int core_cpu_init(struct core_cpu **pcpu, struct core_mmu *mmu)
{
    struct core_cpu *cpu;
    
    *pcpu = NULL;
    *pcpu = malloc(sizeof(struct core_cpu));
    if(*pcpu == NULL) {
        LOGE("Could not allocate cpu core; exiting");
        return 0;
    }
    cpu = *pcpu;

    cpu->mmu = mmu;
    memset(cpu->r, 0, sizeof(cpu->r));
    cpu->i_cycles = 0;
    cpu->i = malloc(sizeof(struct core_instr));
    if(cpu->i == NULL) {
        LOGE("Could not allocate cpu instruction; exiting");
        return 0;
    }
    memset(cpu->i, 0, sizeof(*cpu->i));

    core_cpu_ops[0x0e] = core_cpu_i_op_not;

    return 1;
}

void core_cpu_destroy(struct core_cpu *cpu)
{
    free(cpu);
    cpu = NULL;
}

/* Execute one cycle of the current instruction. */
void core_cpu_i_cycle(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 0) {
        /* T1: opcode fetch */
        cpu->i->ib0 = core_mmu_readb(cpu->mmu, cpu->r[R_P]);
        cpu->i->ib1 = core_mmu_readb(cpu->mmu, cpu->r[R_P] + 1);
        ++cpu->i_cycles;
        cpu->r[R_P] += 2;
    } else {
        /* T2+: opcode decode + execute */
        if(INSTR_OP(cpu->i) < 4) {
            switch((enum core_instr_name) INSTR_OP(cpu->i)) {
                case OP_NOP:
                    core_cpu_i_op_nop(cpu);
                    break;
                case OP_INT:
                    core_cpu_i_op_int(cpu);
                    break;
                case OP_RTI:
                    core_cpu_i_op_rti(cpu);
                    break;
                case OP_RTS:
                    core_cpu_i_op_rts(cpu);
                    break;
            }
        } else {
            struct core_instr_params params;
            params.size = INSTR_OPSZ(cpu->i);
            params.p = &cpu->r[R_P];
            params.s = &cpu->r[R_S];
            params.f = &cpu->r[R_F];
            params.start_cycle = 0; /* ?? */
            if(cpu->i_cycles == 1) {
                /* First operand 1. */
                switch ((enum core_mode_name) INSTR_AM(cpu->i)) {
                    case AM_DR:
                    case AM_DR_DR:
                    case AM_DR_IR:
                    case AM_DR_DB:
                    case AM_DR_IB:
                    case AM_DR_DW:
                    case AM_DR_IW:
                        params.op1.op16 = &cpu->r[INSTR_RX(cpu->i)];
                        break;
                    case AM_IR:
                    case AM_IR_DR:
                        params.op1.op16 =
                            core_mmu_getwp(cpu->mmu, cpu->r[INSTR_RX(cpu->i)]);
                        break;
                    case AM_DB:
                        params.op1.op8 = &cpu->i->db0;
                        break;
                    case AM_IB:
                    case AM_IB_DR:
                        params.op1.op16 =
                            core_mmu_getwp(cpu->mmu, cpu->r[R_P] + cpu->i->db0);
                        break;
                    case AM_DW:
                        params.op1.op16 = (uint16_t *)&cpu->i->db0;
                        break;
                    case AM_IW:
                    case AM_IW_DR:
                        params.op1.op16 =
                            core_mmu_getwp(cpu->mmu, *(uint16_t *)&cpu->i->db0);
                    default:
                        break;
                }
                /* Now for operand 2. */
                switch ((enum core_mode_name) INSTR_AM(cpu->i)) {
                    case AM_DR_DR:
                    case AM_IR_DR:
                        params.op2.op16 = &cpu->r[INSTR_RX(cpu->i)];
                        break;
                    case AM_IB_DR:
                    case AM_IW_DR:
                        params.op2.op16 = &cpu->r[INSTR_RX(cpu->i)];
                        break;
                    case AM_DR_IR:
                        params.op2.op16 =
                            core_mmu_getwp(cpu->mmu, cpu->r[INSTR_RX(cpu->i)]);
                        break;
                    case AM_DR_DB:
                        params.op2.op8 = &cpu->i->db0;
                        break;
                    case AM_DR_DW:
                        params.op2.op16 = (uint16_t *)&cpu->i->db0;
                        break;
                    case AM_DR_IB:
                        params.op2.op16 =
                            core_mmu_getwp(cpu->mmu, cpu->r[R_P] + cpu->i->db0);
                        break;
                    case AM_DR_IW:
                        params.op2.op16 =
                            core_mmu_getwp(cpu->mmu, *(uint16_t *)&cpu->i->db0);
                        break;
                    default:
                        break;
                }
            }

            switch((enum core_mode_name) INSTR_AM(cpu->i)) {
                case AM_DR:
                    core_cpu_ops[INSTR_OP(cpu->i)](cpu, &params);
                    break;
                case AM_IR:
                    core_cpu_ops[INSTR_OP(cpu->i)](cpu, &params);
                    break;
                case AM_DB:
                    break;
                case AM_IB:
                    break;
                case AM_DW:
                    break;
                case AM_IW:
                    break;
                case AM_DR_DR:
                    break;
                case AM_DR_IR:
                    break;
                case AM_IR_DR:
                    break;
                case AM_DR_DB:
                    break;
                case AM_DR_IB:
                    break;
                case AM_DR_DW:
                    break;
                case AM_DR_IW:
                    break;
                case AM_IB_DR:
                    break;
                case AM_IW_DR:
                    break;
            }
            cpu->i_cycles = 0;
        }
    }
}

/*
 * core_cpu_i_instr
 *
 * Execute all the cycles for the current instruction.
 */
void core_cpu_i_instr(struct core_cpu *cpu)
{
    /* Elapsed cycles are also recorded in the core_cpu structure, but when
     * we want to log the elapsed cycles after the instruction is complete,
     * that counter has been reset. So we record it here too.
     */
    int cycles = 0;

    do {
        core_cpu_i_cycle(cpu);
        ++cycles;
    } while(cpu->i_cycles > 0);
    LOGD("core.cpu: %s (%d cycles)", instrnam[INSTR_OP(cpu->i)], cycles);
}

/*
 ******************************************************************************
 * Instruction implementations
 ******************************************************************************
 */

/*
 * core_cpu_i_op_nop
 *
 * NOP instruction implementation.
 */
void core_cpu_i_op_nop(struct core_cpu *cpu)
{
    --cpu->r[R_P];
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_int
 *
 * INT instruction implementation.
 */
void core_cpu_i_op_int(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        core_mmu_writew(cpu->mmu, cpu->r[R_S], cpu->r[R_P]);
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 2) {
        cpu->r[R_S] -= 2;
        cpu->r[R_F] |= FLAG_I;
        ++cpu->i_cycles;
    } else {
        cpu->r[R_P] = core_mmu_readw(cpu->mmu, 0xfffe);
        cpu->i_cycles = 0;
    }
}

/*
 * core_cpu_i_op_rti
 *
 * RTI instruction implementation.
 */
void core_cpu_i_op_rti(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        cpu->r[R_F] = core_mmu_readw(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 2) {
        cpu->r[R_S] += 2;
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 3) {
        cpu->r[R_P] = core_mmu_readw(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else {
        cpu->r[R_S] += 2;
        cpu->i_cycles = 0;
    }
}

/*
 * core_cpu_i_op_rts
 *
 * RTS instruction implementation.
 */
void core_cpu_i_op_rts(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        cpu->r[R_P] = core_mmu_readw(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else {
        cpu->r[R_S] += 2;
        cpu->i_cycles = 0;
    }
}

void core_cpu_i__jump(struct core_cpu *cpu,
                      struct core_instr_params *params,
                      int flag)
{
    static uint16_t v;
    v = *params->op1.op16;
    /* Indirect reg/byte/word: the memory access uses an extra cycle. */
    if((INSTR_AM(cpu->i) & 1) && cpu->i_cycles == 1) {
        v = core_mmu_readw(cpu->mmu, *params->op1.op16);
        ++cpu->i_cycles;
    } else {
        switch(flag) {
        case FLAG_C:
        case FLAG_Z:
        case FLAG_O:
        case FLAG_N:
            if(*params->f & flag)
                *params->p = v;
            break;
        default:
            *params->p = v;
            break;
        }
        cpu->i_cycles = 0;
    }
}

void core_cpu_i__call(struct core_cpu *cpu,
                      struct core_instr_params *params,
                      int flag)
{
    static uint16_t v;
    v = *params->op1.op16;
    if(cpu->i_cycles == 1) {
        core_mmu_writew(cpu->mmu, cpu->r[R_S], cpu->r[R_P]);
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 2) {
        cpu->r[R_S] -= 2;
        /* Indirect reg/byte/word: the memory access uses an extra cycle. */
        if(INSTR_AM(cpu->i) & 1) {
            v = core_mmu_readw(cpu->mmu, *params->op1.op16);
            ++cpu->i_cycles;
        } else {
            switch(flag) {
            case FLAG_Z:
            case FLAG_C:
            case FLAG_O:
            case FLAG_N:
                if(*params->f & flag)
                    *params->p = v;
                break;
            default:
                *params->p = v;
                break;
            }
            cpu->i_cycles = 0;
        }
    } else if(cpu->i_cycles == 3) {
        *params->p = v;
        cpu->i_cycles = 0;
    }
}

/*
 * core_cpu_i_op_jp
 *
 * JP instruction implementation.
 */
void core_cpu_i_op_jp(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__jump(cpu, params, 0);
}

/*
 * core_cpu_i_op_cl
 *
 * CL instruction implementation.
 */
void core_cpu_i_op_cl(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__call(cpu, params, 0);
}

/*
 * core_cpu_i_op_jz
 *
 * JZ instruction implementation.
 */
void core_cpu_i_op_jz(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__jump(cpu, params, FLAG_Z);
}

/*
 * core_cpu_i_op_cz
 *
 * CZ instruction implementation.
 */
void core_cpu_i_op_cz(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__call(cpu, params, FLAG_Z);
}

/*
 * core_cpu_i_op_jc
 *
 * JC instruction implementation.
 */
void core_cpu_i_op_jc(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__jump(cpu, params, FLAG_C);
}

/*
 * core_cpu_i_op_cc
 *
 * CC instruction implementation.
 */
void core_cpu_i_op_cc(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__call(cpu, params, FLAG_C);
}

/*
 * core_cpu_i_op_jo
 *
 * JO instruction implementation.
 */
void core_cpu_i_op_jo(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__jump(cpu, params, FLAG_O);
}

/*
 * core_cpu_i_op_co
 *
 * CO instruction implementation.
 */
void core_cpu_i_op_co(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__call(cpu, params, FLAG_O);
}

/*
 * core_cpu_i_op_jn
 *
 * JN instruction implementation.
 */
void core_cpu_i_op_jn(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__jump(cpu, params, FLAG_N);
}

/*
 * core_cpu_i_op_cn
 *
 * CN instruction implementation.
 */
void core_cpu_i_op_cn(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_cpu_i__call(cpu, params, FLAG_N);
}

/*
 * core_cpu_i_op_not
 *
 * NOT instruction implementation.
 */
void core_cpu_i_op_not(struct core_cpu *cpu, struct core_instr_params *params)
{
    static uint16_t *v;
    v= params->op1.op16;
    /* Indirect reg/byte/word: the memory access uses an extra cycle. */
    if((INSTR_AM(cpu->i) & 1) && cpu->i_cycles == 1) {
        v = core_mmu_getwp(cpu->mmu, *params->op1.op16);
        ++cpu->i_cycles;
    } else {
        *v = ~*v;
        cpu->i_cycles = 0;
    }
}

/*
 * core_cpu_i_op_inc
 *
 * INC instruction implementation.
 */
void core_cpu_i_op_inc(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 += 1;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_dec
 *
 * DEC instruction implementation.
 */
void core_cpu_i_op_dec(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 -= 1;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_ind
 *
 * IND instruction implementation.
 */
void core_cpu_i_op_ind(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 += 2;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_ded
 *
 * DED instruction implementation.
 */
void core_cpu_i_op_ded(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 -= 2;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_mv
 *
 * MV instruction implementation.
 */
void core_cpu_i_op_mv(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_cmp
 *
 * CMP instruction implementation.
 */
void core_cpu_i_op_cmp(struct core_cpu *cpu, struct core_instr_params *params)
{
    int temp = *params->op1.op16 - *params->op2.op16;
    *params->f = 0;
    *params->f |= !temp;
    // TODO: rest of the flags

    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_tst
 *
 * TST instruction implementation.
 */
void core_cpu_i_op_tst(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_add
 *
 * ADD instruction implementation.
 */
void core_cpu_i_op_add(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

/*
 * core_cpu_i_op_sub
 *
 * SUB instruction implementation.
 */
void core_cpu_i_op_sub(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

