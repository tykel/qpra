/*
 * core/core.c -- Emulator core functions.
 *
 * The emulator core are system-wide functions, provide relatively high-level
 * functionality as a result.
 * All the components are managed from here.
 *
 */

#include <stdlib.h>
#include "core/core.h"
#include "core/cpu/cpu.h"
//#include "core/apu/apu.h"
//#include "core/vpu/vpu.h"
//#include "core/mmu/mmu.h"
//#include "core/cart/cart.h"
//#include "core/pad/pad.h"
#include "log.h"

void *core_entry(void *data)
{
    struct core_system *core;

    core_init(&core);

    LOGD("Beginning emulation");
    LOGD("Finished emulation");
    return NULL;
}

void core_init(struct core_system **core)
{
    *core = malloc(sizeof(struct core_system));
    //core_apu_init((*core)->apu);
    //core_vpu_init((*core)->vpu);
    //core_mmu_init((*core)->mmu);
    //core_cart_init((*core)->cart);
    //core_pad_init((*core)->pad);
    core_cpu_init((*core)->cpu, (*core)->mmu);
    LOGD("Core initialized");
    return;
}
