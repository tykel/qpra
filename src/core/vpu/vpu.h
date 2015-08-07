#ifndef __vpu_h__
#define __vpu_h__

#include "common.h"

#define NUM_SPRITES     64
#define PALETTE_SIZE    16
#define TILE_SIZE       (4*8)
#define TILES_W_MEM     36
#define TILES_W_VIS     32
#define TILES_H_MEM     28
#define TILES_H_VIS     32
#define VIS_PIXELS      256
#define VIS_SCANLINES   224

struct vpu_sprite {
    bool enabled;
    int z;
    int hh;
    int vv;
    uint8_t group;
    uint8_t xoffs;
    uint8_t yoffs;
    uint8_t tile;

    // Convenience
    int x_start, x_end;
    int y_start, y_end;
};

struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct vpu_state {
    struct vpu_sprite sprites[NUM_SPRITES];
    struct rgba sprite_palette[PALETTE_SIZE];
    struct rgba tl1_palette[PALETTE_SIZE];
    struct rgba tl2_palette[PALETTE_SIZE];

    struct rgba global_palette[16 * PALETTE_SIZE];

    int cycle;
    int scanline;
    struct cpu_state *cpu;
};

bool vpu_init(struct vpu_state *, struct cpu_state *);
bool vpu_cycle(struct vpu_state *);
bool vpu_destroy(struct vpu_state *);

#endif
