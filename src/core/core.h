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

#pragma pack(push, 1)
struct core_header_map
{
    char magic[4];
    uint32_t size;
    uint32_t crc32;
    uint8_t rom_banks;
    uint8_t ram_banks;
    uint8_t tile_banks;
    uint8_t dpcm_banks;
    uint32_t reserved;
    char name[16];
    char desc[32];

    uint8_t *data;
};
#pragma pack(pop)

struct core_system
{
    struct core_cpu *cpu;
    struct core_apu *apu;
    struct core_vpu *vpu;
    struct core_mmu *mmu;
    struct core_cart *cart;
    struct core_pad *pad;

    struct core_header_map *header;
};

void *core_entry(void *);
int core_init(struct core_system *);
static int core_load_rom(struct core_system *, const char *);
static int core_load_palette(struct core_system *, uint8_t *);

#endif
