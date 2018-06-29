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
#include <unistd.h>
#include "core/core.h"
#include "core/cpu/cpu.h"
//#include "core/apu/apu.h"
#include "core/vpu/vpu.h"
//#include "core/mmu/mmu.h"
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
    struct timespec tslf;
    int frame = 0;
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

    clock_gettime(CLOCK_MONOTONIC_RAW, &tslf);

    LOGD("Beginning emulation");
    while(!done()) {
        vpu_cycle(&core->vpu);
        cpu_cycle(&core->cpu);
        cycles += 1;

        if(cycles >= CORE_CYCLES_F) {
            long unsigned int elapsed_us;
            struct timespec tscf;

            clock_gettime(CLOCK_MONOTONIC_RAW, &tscf);
            elapsed_us = (tscf.tv_sec - tslf.tv_sec)*1000000 + (tscf.tv_nsec - tslf.tv_nsec)/1000;
            if (elapsed_us < 16666) {
                unsigned int usecs_wait = 16666 - elapsed_us;
                LOGD("frame %d: elapsed %ld us, sleeping %ld us", frame, elapsed_us, usecs_wait);
                usleep(usecs_wait);
            } else {
                LOGD("frame %d: elapsed %ld us", frame, elapsed_us);
            }
            cycles = 0;
            ++frame;
            tslf = tscf;
        }
    }

    LOGD("Finished emulation");

    LOGD("Emulation core thread exiting");
}


/*
 * Top-level initialization routine.
 * Initializes the various devices in core_system, turn by turn.
 */
int core_init(struct core_system *core, struct core_temp_banks *banks)
{
//     struct core_mmu_params mmup;
    uint8_t palette[768];

//     mmup.rom_banks = core->header->rom_banks;
//     mmup.ram_banks = core->header->ram_banks;
//     mmup.tile_banks = core->header->tile_banks;
//     mmup.dpcm_banks = core->header->dpcm_banks;
//     if(!core_mmu_init(&core->mmu, &mmup, banks))
//         return 0;

    if(!cpu_init(&core->cpu))
        return 0;
    if(!core_load_palette(core, palette))
        return 0;
    if(!vpu_init_palette(&core->vpu, palette))
        return 0;
    if(!vpu_init(&core->vpu, &core->cpu))
        return 0;
    //core_apu_init(core->apu);
    //core_cart_init(core->cart);
    //core_pad_init(core->pad);

    memcpy(core->cpu.m, banks->rom_f, 16*1024);

    LOGD("Core initialized");
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

