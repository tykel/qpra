/*
 * core/core.h -- Emulator core functions (header).
 *
 * Declares the system structure, which holds the emulator core components, and
 * the core functions.
 *
 */

#ifndef QPRA_CORE_H
#define QPRA_CORE_H

#include <stdint.h>

struct core_system
{
    struct core_cpu *cpu;
    struct core_apu *apu;
    struct core_vpu *vpu;
    struct core_mmu *mmu;
    struct core_cart *cart;
    struct core_pad *pad;
};

void *core_entry(void *);
int core_init(struct core_system **);

#endif
