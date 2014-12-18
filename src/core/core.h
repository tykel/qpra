
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
void core_init(struct core_system **);

#endif
