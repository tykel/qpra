/*
 * core/cpu/cpu.c -- Emulator CPU functions.
 *
 * The CPU functions, notably the instructions, are implemented here.
 *
 */

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
            switch((enum core_mode_name) INSTR_AM(cpu->i)) {
                case AM_DR:
                    core_cpu_ops[INSTR_OP(cpu->i)](&cpu->r[INSTR_RX(cpu->i)]);
                    break;
                case AM_IR:
                    core_cpu_ops[INSTR_OP(cpu->i)](&cpu->r[INSTR_RX(cpu->i)]);
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
    do {
        core_cpu_i_cycle(cpu);
    } while(cpu->i_cycles > 0);
}

void core_cpu_i_op_nop(struct core_cpu *cpu)
{
    ++cpu->r[R_P];
    cpu->i_cycles = 0;
}

void core_cpu_i_op_int(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        core_mmu_writeb(cpu->mmu, cpu->r[R_S], cpu->r[R_P]);
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 2) {
        cpu->r[R_S] -= 2;
        cpu->r[R_F] |= 0x10;
        ++cpu->i_cycles;
    } else {
        cpu->r[R_P] = core_mmu_readb(cpu->mmu, 0xfffe);
        cpu->i_cycles = 0;
    }
}

void core_cpu_i_op_rti(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        cpu->r[R_F] = core_mmu_readb(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 2) {
        cpu->r[R_S] += 2;
        ++cpu->i_cycles;
    } else if(cpu->i_cycles == 3) {
        cpu->r[R_P] = core_mmu_readb(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else {
        cpu->r[R_S] += 2;
        cpu->i_cycles = 0;
    }
}

void core_cpu_i_op_rts(struct core_cpu *cpu)
{
    if(cpu->i_cycles == 1) {
        cpu->r[R_P] = core_mmu_readb(cpu->mmu, cpu->r[R_S]);
        ++cpu->i_cycles;
    } else {
        cpu->r[R_S] += 2;
        cpu->i_cycles = 0;
    }
}

