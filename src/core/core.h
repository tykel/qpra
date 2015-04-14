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

#define CORE_CYCLES_S               5360520

#ifdef _DEBUG
#define CORE_CYCLES_F               20
#define CORE_CYCLES_VBLANK          10
#define CORE_CYCLES_F_PRE_VBLANK    10
#else
#define CORE_CYCLES_F               89342
#define CORE_CYCLES_VBLANK          1023
#define CORE_CYCLES_F_PRE_VBLANK    88319
#endif


enum core_buf_type {
    CORE_HDR_ROMF=0, CORE_HDR_ROMS=1, CORE_HDR_RAMF=2,
    CORE_HDR_RAMS=3, CORE_HDR_TILS=4, CORE_HDR_AUDS=5
};

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

#pragma pack(push, 1)
struct core_header_bufmap
{
    uint8_t type;
    uint8_t num;
    uint16_t len;
};
#pragma pack(pop)

struct core_temp_banks
{
    uint8_t *rom_f;
    uint8_t *rom_s[256];
    uint8_t *ram_f;
    uint8_t *ram_s[256];
    uint8_t *tile_s[256];
    uint8_t *dpcm_s[256];
};

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
int core_init(struct core_system *, struct core_temp_banks *);
static int core_load_rom(struct core_system *, const char *,
        struct core_temp_banks *);
static int core_load_palette(struct core_system *, uint8_t *);

#endif
