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

void core_cpu_i_instr(struct core_cpu *cpu)
{
    int cycles = 0;


    do {
        core_cpu_i_cycle(cpu);
        ++cycles;
    } while(cpu->i_cycles > 0);
    LOGD("core.cpu: processed %d-cycle instr.", cycles);
}

void core_cpu_i_op_nop(struct core_cpu *cpu)
{
    LOGD("core.cpu: nop");
    cpu->i_cycles = 0;
}

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

void core_cpu_i_op_jp(struct core_cpu *cpu, struct core_instr_params *params)
{
    static uint8_t v;
    v = *params->op1.op16;
    /* Indirect reg/byte/word: the memory access uses an extra cycle. */
    if((INSTR_AM(cpu->i) & 1) && cpu->i_cycles == 1) {
        v = core_mmu_readw(cpu->mmu, *params->op1.op16);
        ++cpu->i_cycles;
    } else {
        *params->p = v;
        cpu->i_cycles = 0;
    }
}

void core_cpu_i_op_cl(struct core_cpu *cpu, struct core_instr_params *params)
{
    core_mmu_writew(cpu->mmu, *params->s, *params->p);
    *params->s -= 2;
    *params->p = *params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_jz(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_Z)
        *params->p = *params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_cz(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_Z) {
        core_mmu_writew(cpu->mmu, *params->s, *params->p);
        *params->s -= 2;
        *params->p = *params->op1.op16;
    }
    cpu->i_cycles = 0;
}

void core_cpu_i_op_jc(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_C)
        *params->p = *params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_cc(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_C) {
        core_mmu_writew(cpu->mmu, *params->s, *params->p);
        *params->s -= 2;
        *params->p = *params->op1.op16;
    }
    cpu->i_cycles = 0;
}

void core_cpu_i_op_jo(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_O)
        *params->p = *params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_co(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_O) {
        core_mmu_writew(cpu->mmu, *params->s, *params->p);
        *params->s -= 2;
        *params->p = *params->op1.op16;
    }
    cpu->i_cycles = 0;
}

void core_cpu_i_op_jn(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_N)
        *params->p = *params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_cn(struct core_cpu *cpu, struct core_instr_params *params)
{
    if(*params->f & FLAG_N) {
        core_mmu_writew(cpu->mmu, *params->s, *params->p);
        *params->s -= 2;
        *params->p = *params->op1.op16;
    }
    cpu->i_cycles = 0;
}

void core_cpu_i_op_not(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = ~*params->op1.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_inc(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 += 1;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_dec(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 -= 1;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_ind(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 += 2;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_ded(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 -= 2;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_mv(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_cmp(struct core_cpu *cpu, struct core_instr_params *params)
{
    int temp = *params->op1.op16 - *params->op2.op16;
    *params->f = 0;
    *params->f |= !temp;
    // TODO: rest of the flags

    cpu->i_cycles = 0;
}

void core_cpu_i_op_tst(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_add(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

void core_cpu_i_op_sub(struct core_cpu *cpu, struct core_instr_params *params)
{
    *params->op1.op16 = *params->op2.op16;
    cpu->i_cycles = 0;
}

