#include <stdlib.h>
#include "core/core.h"
#include "core/cpu/cpu.h"
//#include "core/apu/apu.h"
//#include "core/vpu/vpu.h"
//#include "core/mmu/mmu.h"
//#include "core/cart/cart.h"
//#include "core/pad/pad.h"

void *core_entry(void *data)
{
    return NULL;
}

void core_init(struct core_system **core)
{
    *core = malloc(sizeof(struct core_system));
    core_cpu_init((*core)->cpu);
    //core_apu_init((*core)->apu);
    //core_vpu_init((*core)->vpu);
    //core_mmu_init((*core)->mmu);
    //core_cart_init((*core)->cart);
    //core_pad_init((*core)->pad);
    return;
}
