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
#include <time.h>
#include "core/core.h"
#include "core/cpu/cpu.h"
//#include "core/apu/apu.h"
#include "core/vpu/vpu.h"
#include "core/mmu/mmu.h"
//#include "core/cart/cart.h"
//#include "core/pad/pad.h"
#include "log.h"

int done();

const char *palette_fn = "palette.bin";

struct arg_pair
{
    int argc;
    char **argv;
};


/*
 * Emulation thread entry point.
 * Parses the command line, loads the ROM (if any) and begins emulation.
 */
void *core_entry(void *data)
{
    struct core_system *core;
    struct core_temp_banks banks;
    struct timespec ts0, ts1, ts_sleep;
    unsigned int frame = 0;
    intmax_t us, us_sum = 0;
    int cycles = 0;

    struct arg_pair *pair = (struct arg_pair *)data;
    
    core = malloc(sizeof(struct core_system));
    if(core == NULL) {
        LOGE("Could not allocate core structure");
        return 0;
    }

    if(pair->argv[1][0] != '-' && core_load_rom(core, pair->argv[1], &banks)) {
        LOGD("Loaded ROM file '%s' successfully", pair->argv[1]);
    } else {
        LOGD("Couldn't load a ROM file");
    }

    if(!core_init(core, &banks)) {
        LOGE("System initialization failed; exiting");
        return NULL;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts0);
    LOGD("Beginning emulation");
    while(!done()) {
        uint16_t pc = core->cpu->r[R_P];
        core->cpu->i_cycles = 0;
        core->cpu->i_done = 0;
        core->cpu->i_middle = 0;
        
        do {
            /* Apply any pending read/write requests on the bus. */
            core_mmu_update(core->cpu->mmu);
            /* Execute a cycle in the VPU. */
            core_vpu_cycle(core->vpu, core->cpu->total_cycles);
            /* Execute an instruction cycle in the CPU. */
            core_cpu_i_cycle(core->cpu);
            LOGV("core.cpu: ... cycle %d", core->cpu->i_cycles);

            cycles += 1;
        } while(!core->cpu->i_done);

        LOGV("core.cpu: %04x: %s (%d cycles) (p now %04x)",
             pc, instrnam[INSTR_OP(core->cpu->i)], core->cpu->i_cycles, core->cpu->r[R_P]);
#ifdef _DEBUG
        getc(stdin);
#else
        
        /* One frame's worth of cycles have been executed, so time to pause. */
        if(cycles >= CORE_CYCLES_F) {

            clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
            us = (ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000) -
                 (ts0.tv_sec * 1000000 + ts0.tv_nsec / 1000);
            us_sum += us;
        
            LOGV("  frame: % 3d.%03d ms", us / 1000, us % 1000);
            if(++frame == 60) {
                us_sum /= 60;
                LOGD("frame avg: % 3d.%03d ms", us_sum / 1000, us_sum % 1000);
                us_sum = frame = 0;
            }

            if(us < 16666) {
                ts_sleep.tv_sec = 0;
                ts_sleep.tv_nsec = 16666666 - (us * 1000);
                LOGV("sleeping %d ns", ts_sleep.tv_nsec);
                nanosleep(&ts_sleep, NULL);
            }
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts0);
            cycles = 0;
        }
#endif
    }
    LOGD("Finished emulation");
    core_destroy(core);
    free(core);

    LOGD("Emulation core thread exiting");
}


/* 
 * Top-level initialization routine.
 * Initializes the various devices in core_system, turn by turn.
 */
int core_init(struct core_system *core, struct core_temp_banks *banks)
{
    struct core_mmu_params mmup;
    uint8_t palette[768];
    
    mmup.rom_banks = core->header->rom_banks;
    mmup.ram_banks = core->header->ram_banks;
    mmup.tile_banks = core->header->tile_banks;
    mmup.dpcm_banks = core->header->dpcm_banks;
    if(!core_mmu_init(&core->mmu, &mmup, banks))
        return 0;
    
    if(!core_cpu_init(&core->cpu, core->mmu))
        return 0;
    if(!core_mmu_cpu(core->mmu, core->cpu))
        return 0;
    if(!core_vpu_init(&core->vpu, core->cpu))
        return 0;
    if(!core_mmu_vpu(core->mmu, core->vpu))
        return 0;
    if(!core_load_palette(core, palette))
        return 0;
    if(!core_vpu_init_palette(core->vpu, palette))
        return 0;
    //core_apu_init(core->apu);
    //core_cart_init(core->cart);
    //core_pad_init(core->pad);
    
    LOGD("Core initialized");
    return 1;
}

int core_destroy(struct core_system *core)
{
    core_vpu_destroy(core->vpu);
    core_mmu_destroy(core->mmu);
    core_cpu_destroy(core->cpu);
    return 1;
}


/* Reads and parses the ROM from disk. */
int core_load_rom(struct core_system *core, const char *fn,
        struct core_temp_banks *banks)
{
    struct core_header_map *map;
    struct core_header_bufmap buf;
    uint8_t *data;
    int total = 0;
    FILE *fp = NULL;

    map = malloc(sizeof(struct core_header_map));
    fp = fopen(fn, "rb");
    if(fp == NULL) {
        LOGE("Couldn't open ROM file '%s'", fn);
        return 0;
    }
    if((total = fread(map, sizeof(uint8_t), 68, fp)) != 68) {
        LOGE("Couldn't read full ROM header");
        return 0;
    }
    
    do {
        uint8_t *dst;
        total += fread(&buf, sizeof(uint8_t), sizeof(buf), fp);
        switch(buf.type) {
            case CORE_HDR_ROMF:
                banks->rom_f = malloc(buf.len);
                dst = banks->rom_f;
                break;
            case CORE_HDR_ROMS:
                banks->rom_s[buf.num] = malloc(buf.len);
                dst = banks->rom_s[buf.num];
                break;
            case CORE_HDR_RAMF:
                banks->ram_f = malloc(buf.len);
                dst = banks->ram_f;
                break;
            case CORE_HDR_RAMS:
                banks->ram_s[buf.num] = malloc(buf.len);
                dst = banks->ram_s[buf.num];
                break;
            case CORE_HDR_TILS:
                banks->tile_s[buf.num] = malloc(buf.len);
                dst = banks->tile_s[buf.num];
                break;
            case CORE_HDR_AUDS:
                banks->dpcm_s[buf.num] = malloc(buf.len);
                dst = banks->dpcm_s[buf.num];
                break;
            default:
                LOGE("Invalid buffer type found 0x%02x", buf.type);
                return 0;
        }
        total += fread(dst, sizeof(uint8_t), buf.len, fp);

    } while(total < map->size);
    
    fclose(fp);

    LOGD("Header: size: %d, rom banks: %d, ram banks: %d, tile banks: %d, "
         "dpcm banks: %d",
         map->size, map->rom_banks, map->ram_banks, map->tile_banks,
         map->dpcm_banks);
    LOGD("Header: name: '%s', description: '%s'", map->name, map->desc);

    core->header = map;

    return 1;
}


/* Read the palette into memory for the VPU. */
int core_load_palette(struct core_system *core, uint8_t *buffer)
{
    int n;
    FILE *fp = NULL;
    
    fp = fopen(palette_fn, "rb");
    if(fp == NULL) {
        LOGE("Couldn't open palette in '%s'", palette_fn);
        return 0;
    }
    n = fread(buffer, 1, 768, fp);
    if(n != 768) {
        LOGE("Couldn't read full palette file");
        return 0;
    }
    fclose(fp);

    LOGD("Read in palette file: size: 768");
    return 1;
}

