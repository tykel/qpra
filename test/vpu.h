#ifndef __vpu_h__
#define __vpu_h__

#include "common.h"
#include "cpu.h"

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

struct rgba *fb;

inline int coarse_scroll_l1h(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb82] + 32) % 36; }
inline int coarse_scroll_l1v(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb84] + 28) % 32; }
inline int coarse_scroll_l2h(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb86] + 32) % 36; }
inline int coarse_scroll_l2v(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb88] + 28) % 32; }

inline int fine_scroll_l1h(struct vpu_state *vpu) { return vpu->cpu->m[0xeb83] & 7; }
inline int fine_scroll_l1v(struct vpu_state *vpu) { return vpu->cpu->m[0xeb85] & 7; }
inline int fine_scroll_l2h(struct vpu_state *vpu) { return vpu->cpu->m[0xeb87] & 7; }
inline int fine_scroll_l2v(struct vpu_state *vpu) { return vpu->cpu->m[0xeb89] & 7; }

inline int scanline_y(int scanline) { return scanline - 16; }
inline int cycle_x(int cycle) { return cycle - 25; }

inline bool in_vblank(int scanline) { return scanline <= 12 || scanline >= 240; }
inline bool in_hsync(int cycle) { return cycle <= 25; }
inline bool in_bp_cb(int cycle) { return cycle >= 26 && cycle <= 64; }

bool vpu_init(struct vpu_state *, struct cpu_state *);
bool vpu_cycle(struct vpu_state *);
bool vpu_destroy(struct vpu_state *);

#endif
