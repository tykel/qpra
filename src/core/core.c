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
#include "core/mmu/mmu.h"
//#include "core/cart/cart.h"
//#include "core/pad/pad.h"
#include "log.h"

void *core_entry(void *data)
{
    struct core_system *core;

    if(!core_init(&core)) {
        LOGE("System initialization failed; exiting");
        return NULL;
    }

    LOGD("Beginning emulation");
    // Actual emulation here
    LOGD("Finished emulation");
    return NULL;
}

int core_init(struct core_system **core)
{
    struct core_mmu_params mmup;
    
    *core = malloc(sizeof(struct core_system));
    if(*core == NULL) {
        LOGE("Could not allocate core structure");
        return 0;
    }
    
    //core_apu_init((*core)->apu);
    //core_vpu_init((*core)->vpu);
    
    mmup.rom_banks = 1;
    mmup.ram_banks = 1;
    mmup.tile_banks = 1;
    mmup.dpcm_banks = 1;
    mmup.vpu_readb = mmup.apu_readb = (void *) 0xffff0000;
    mmup.vpu_writeb = mmup.apu_writeb = (void *) 0xffff0000;
    if(!core_mmu_init(&(*core)->mmu, &mmup))
        return 0;
    
    //core_cart_init((*core)->cart);
    //core_pad_init((*core)->pad);
    
    if(!core_cpu_init(&(*core)->cpu, (*core)->mmu))
        return 0;
    
    LOGD("Core initialized");
    return 1;
}
