#include <stdio.h>
#include <stdint.h>

#include "vpu.h"
#include "cpu.h"

// FIXME: Temp until this code is integrated back into the emulator
uint8_t framebuffer[VIS_PIXELS * VIS_SCANLINES * 4];

struct rgba *fb;


static inline int coarse_scroll_l1h(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb82] + 32) % 36; }
static inline int coarse_scroll_l1v(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb84] + 28) % 32; }
static inline int coarse_scroll_l2h(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb86] + 32) % 36; }
static inline int coarse_scroll_l2v(struct vpu_state *vpu) { return (vpu->cpu->m[0xeb88] + 28) % 32; }

static inline int fine_scroll_l1h(struct vpu_state *vpu) { return vpu->cpu->m[0xeb83] & 7; }
static inline int fine_scroll_l1v(struct vpu_state *vpu) { return vpu->cpu->m[0xeb85] & 7; }
static inline int fine_scroll_l2h(struct vpu_state *vpu) { return vpu->cpu->m[0xeb87] & 7; }
static inline int fine_scroll_l2v(struct vpu_state *vpu) { return vpu->cpu->m[0xeb89] & 7; }

static inline int scanline_y(int scanline) { return scanline - 16; }
static inline int cycle_x(int cycle) { return cycle - 25; }

static inline bool in_vblank(int scanline) { return scanline <= 12 || scanline >= 240; }
static inline bool in_hsync(int cycle) { return cycle <= 25; }
static inline bool in_bp_cb(int cycle) { return cycle >= 26 && cycle <= 64; }


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

void tmap1_pal_get(struct vpu_state *vpu)
{
    int i;
    uint8_t tp = vpu->cpu->m[0xeb40] & 0x0f;
    uint8_t *p = &vpu->cpu->m[0xe900 + tp*PALETTE_SIZE];

    for(i = 0; i < PALETTE_SIZE; ++i)
        vpu->tl1_palette[i] = vpu->global_palette[p[i]];
}

struct rgba tl1_px(struct vpu_state *vpu, int x, int y)
{
    int tx = x >> 3;
    int ty = y >> 3;
    int t = vpu->cpu->m[0xe000 + ty*TILES_H_MEM + tx];
    int c = vpu->cpu->m[0xc000 + t*TILE_SIZE + y*4 + (x>>1)];
    c = (c & 1) ? (c & 0xf) : (c >> 4);
    return vpu->tl1_palette[c];
}

void tl2_pal_get(struct vpu_state *vpu)
{
    int i;
    uint8_t tp = vpu->cpu->m[0xeb40] >> 4;
    uint8_t *p = &vpu->cpu->m[0xe900 + tp*PALETTE_SIZE];

    for(i = 0; i < PALETTE_SIZE; ++i)
        vpu->tl2_palette[i] = vpu->global_palette[p[i]];
}

struct rgba tl2_px(struct vpu_state *vpu, int x, int y)
{
    int tx = x >> 3;
    int ty = y >> 3;
    int t = vpu->cpu->m[0xe000 + ty*TILES_H_MEM + tx];
    int c = vpu->cpu->m[0xc000 + t*TILE_SIZE + y*4 + (x>>1)];
    c = (c & 1) ? (c & 0xf) : (c >> 4);
    return vpu->tl2_palette[c];
}

bool vpu_init(struct vpu_state *vpu, struct cpu_state *cpu)
{
    fb = (struct rgba *)framebuffer;
    vpu->cpu = cpu;
    return true;
}

bool vpu_cycle(struct vpu_state *vpu)
{
    struct rgba tl1, tl2, s[NUM_SPRITES];
    int i;
    int x = cycle_x(vpu->cycle);
    int y = scanline_y(vpu->scanline);

    printf("% 6d (vpu) x = %d, y = %d\n", vpu->cycle, x, y);

    // In VBlank scanlines or the non-visible part of the scanline, do nothing.
    if(in_vblank(vpu->scanline))
        goto __vpu_cycle_inc;
    if(in_hsync(vpu->cycle) || in_bp_cb(vpu->cycle))
        goto __vpu_cycle_inc;

    // We are in a visible scanline, and in the active part of the scanline.
    // Get the right pixel and output it.
    tl1 = tl1_px(vpu, x, y);
    tl2 = tl2_px(vpu, x, y);
    for(i = 0; i < NUM_SPRITES; ++i) {
        s[i] = sprite_px(vpu, i, x, y);
    }
    // TODO: Some magic, to determine which pixel layer to display
    fb[y * VIS_PIXELS + x] = tl2;

__vpu_cycle_inc:
    // Increment (and wrap if necessary) the cycle and scanline counters.
    vpu->cycle += 1;
    if(vpu->cycle > 340) {
        vpu->cycle = 0;
        vpu->scanline += 1;
    }
    if(vpu->scanline > 261) {
        vpu->scanline = 0;
    }

    return true;
}

bool vpu_destroy(struct vpu_state *vpu)
{
    return true;
}
