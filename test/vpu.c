#include <stdio.h>
#include <stdint.h>

#include "cpu.h"

#define NUM_SPRITES     64
#define PALETTE_SIZE    16
#define TILE_SIZE       (4*8)

#define false 0
#define true 1
typedef int bool;

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
    struct rgba tmap1_palette[PALETTE_SIZE];
    struct rgba tmap2_palette[PALETTE_SIZE];

    struct rgba global_palette[16 * PALETTE_SIZE];

    struct cpu_state *cpu;
};

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

// Perform a clean read of the sprite from memory
void sprite_get(struct vpu_state *vpu, int i)
{
    uint8_t *sm = &vpu->cpu->m[0xea00 + 4*i];
    struct vpu_sprite *s = &vpu->sprites[i];

    s->enabled = !!(sm[0] & 0x80);
    s->z = (sm[0] & 0x70) >> 4;
    s->hh = !!(sm[0] & 4);
    s->vv = sm[0] & 1;
    s->group = sm[1] & 0x4f;
    s->xoffs = (sm[2] >> 4) * 8;
    s->yoffs = (sm[2] & 0x0f) * 8;
    s->tile = sm[3];

    s->x_start = vpu->cpu->m[0xeb00 + s->group*2];
    s->x_end = s->x_start + (8 << s->hh);
    s->y_start = vpu->cpu->m[0xeb01 + s->group*2];
    s->y_end = s->y_start + (8 << s->vv);
}

void sprite_pal_get(struct vpu_state *vpu)
{
    int i;
    uint8_t sp = vpu->cpu->m[0xeb81] & 0x0f;
    uint8_t *p = &vpu->cpu->m[0xe900 + sp*PALETTE_SIZE];

    for(i = 0; i < PALETTE_SIZE; ++i)
        vpu->sprite_palette[i] = vpu->global_palette[p[i]];
}

struct rgba sprite_px(struct vpu_state *vpu, int i, int x, int y)
{
    struct vpu_sprite *s = &vpu->sprites[i];
    int c = vpu->cpu->m[0xc000 + s->tile*TILE_SIZE + y*4 + (x>>1)];
    c = (x & 1) ? (c & 0xf) : (c >> 4);
    return vpu->sprite_palette[c];
}
