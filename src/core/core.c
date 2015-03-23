/*
 * core/core.c -- Emulator core functions.
 *
 * The emulator core are system-wide functions, provide relatively high-level
 * functionality as a result.
 * All the components are managed from here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/core.h"
#include "core/cpu/cpu.h"
//#include "core/apu/apu.h"
//#include "core/vpu/vpu.h"
#include "core/mmu/mmu.h"
//#include "core/cart/cart.h"
//#include "core/pad/pad.h"
#include "log.h"

int done();

struct arg_pair
{
    int argc;
    char **argv;
};

void *core_entry(void *data)
{
    struct core_system *core;

    struct arg_pair *pair = (struct arg_pair *)data;
    
    core = malloc(sizeof(struct core_system));
    if(core == NULL) {
        LOGE("Could not allocate core structure");
        return 0;
    }

    if(pair->argv[1][0] != '-' && core_load_rom(core, pair->argv[1])) {
        LOGD("Loaded ROM file '%s' successfully", pair->argv[1]);
    } else {
        LOGD("Couldn't load a ROM file");
    }

    if(!core_init(core)) {
        LOGE("System initialization failed; exiting");
        return NULL;
    }

    memcpy(core->mmu->rom_f, core->header->data,
            core->header->size - sizeof(core->header) - sizeof(uint8_t *));

    LOGD("Beginning emulation");
    while(!done()) {
        for(int i = 0; i < 4; ++i)
            core_cpu_i_instr(core->cpu);
        getc(stdin);
    }
    LOGD("Finished emulation");

    LOGD("Emulation core thread exiting");
}

int core_init(struct core_system *core)
{
    struct core_mmu_params mmup;
    
    //core_apu_init(core->apu);
    //core_vpu_init(core->vpu);
    
    mmup.rom_banks = core->header->rom_banks;
    mmup.ram_banks = core->header->ram_banks;
    mmup.tile_banks = core->header->tile_banks;
    mmup.dpcm_banks = core->header->dpcm_banks;
    mmup.vpu_readb = mmup.apu_readb = (void *) 0xffff0000;
    mmup.vpu_writeb = mmup.apu_writeb = (void *) 0xffff0000;
    if(!core_mmu_init(&core->mmu, &mmup))
        return 0;
    
    //core_cart_init(core->cart);
    //core_pad_init(core->pad);
    
    if(!core_cpu_init(&core->cpu, core->mmu))
        return 0;
    if(!core_mmu_cpu(core->mmu, core->cpu))
        return 0;
    
    LOGD("Core initialized");
    return 1;
}

int core_load_rom(struct core_system *core, const char *fn)
{
    struct core_header_map *map;
    uint8_t *data;
    FILE *fp = NULL;

    map = malloc(sizeof(struct core_header_map));
    fp = fopen(fn, "rb");
    if(fp == NULL) {
        LOGE("Couldn't open ROM file '%s'", fn);
        return 0;
    }
    if(fread(map, sizeof(uint8_t), 68, fp) != 68) {
        LOGE("Couldn't read full ROM header");
        return 0;
    }
    data = malloc(map->size - 68);
    if(fread(data, sizeof(uint8_t), map->size - 68, fp) != (map->size - 68)) {
        LOGE("Couldn't read full ROM file");
        return 0;
    }
    map->data = data;
    fclose(fp);

    LOGD("Header: size: %d, rom banks: %d, ram banks: %d, tile banks: %d, "
         "dpcm banks: %d",
         map->size, map->rom_banks, map->ram_banks, map->tile_banks,
         map->dpcm_banks);
    LOGD("Header: name: '%s', description: '%s'", map->name, map->desc);

    core->header = map;

    return 1;
}

