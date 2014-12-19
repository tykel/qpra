/*
 * core/cpu/cpu.c -- Emulator CPU functions.
 *
 * The CPU functions, notably the instructions, are implemented here.
 *
 */

#include <stdlib.h>
#include "core/cpu/cpu.h"
#include "log.h"

void core_cpu_init(struct core_cpu *cpu, struct core_mmu *mmu)
{
    cpu = NULL;
    
    cpu = malloc(sizeof(struct core_cpu));
    if(cpu == NULL) {
        LOGE("Could not allocate cpu core; exiting");
        exit(1);
    }

    cpu->mmu = mmu;
}

void core_cpu_destroy(struct core_cpu *cpu)
{
    free(cpu);
    cpu = NULL;
}


void core_cpu_i_cycle(struct core_cpu *cpu)
{
    if(INSTR_OP(cpu->i) < 4) {
        switch((enum core_instr_name) INSTR_OP(cpu->i)) {
            case OP_NOP:
                break;
            case OP_INT:
            case OP_RTI:
                break;
            case OP_RTS:
                break;
        }
    } else {
        switch((enum core_mode_name) INSTR_AM(cpu->i)) {
            case AM_DR:
            case AM_DR_DR:
                break;
            case AM_DB:
            case AM_DW:
            case AM_DR_DB:
            case AM_DR_DW:
                break;
            case AM_IR:
            case AM_IB:
            case AM_IW:
            case AM_DR_IR:
            case AM_IR_DR:
            case AM_DR_IB:
            case AM_DR_IW:
            case AM_IB_DR:
            case AM_IW_DR:
                break;
        }
    }
}

void core_cpu_i_instr(struct core_cpu *cpu)
{
    LOGE("Instruction stepping not yet supported\n");
}

void core_cpu_i_op_nop(struct core_cpu *cpu)
{
    ++(cpu->i_cycles);
}

