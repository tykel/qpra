/*
 * core/vpu/vpu.c -- Emulator VPU functions.
 *
 * The functions used by the Video Processing Unit.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "core/vpu/vpu.h"
#include "core/cpu/cpu.h"
#include "core/mmu/mmu.h"
#include "ui/ui.h"
#include "log.h"

#ifdef _DEBUG
#define _DEBUG_VPU
#define _DEBUG_MEMORY
#endif

struct rgba pal_fixed[VPU_FIX_PALETTE_SZ];


static inline int core_vpu__pal_l1(struct core_vpu *vpu) {
    return (*vpu->layers_pi & VPU_LAYER1_PI) >> 4;
}

static inline int core_vpu__pal_l2(struct core_vpu *vpu) {
    return (*vpu->layers_pi & VPU_LAYER2_PI);
}

static inline int core_vpu__spr_enabled(struct core_vpu_sprite *spr) {
    return !!(spr->b0 & VPU_SPR_ENABLE);
}

static inline int core_vpu__spr_depth(struct core_vpu_sprite *spr) {
    return (spr->b0 & VPU_SPR_DEPTH) >> 4; 
}

static inline int core_vpu__spr_hmirror(struct core_vpu_sprite *spr) {
    return !!(spr->b0 & VPU_SPR_HMIRROR);
}

static inline int core_vpu__spr_vmirror(struct core_vpu_sprite *spr) {
    return !!(spr->b0 & VPU_SPR_VMIRROR);
}

static inline int core_vpu__spr_hdouble(struct core_vpu_sprite *spr) {
    return !!(spr->b0 & VPU_SPR_HDOUBLE);
}

static inline int core_vpu__spr_vdouble(struct core_vpu_sprite *spr) {
    return !!(spr->b0 & VPU_SPR_VDOUBLE);
}

static inline int core_vpu__spr_group(struct core_vpu_sprite *spr) {
    return (spr->b1 & VPU_SPR_GROUP);
}

static inline int core_vpu__spr_xoffs(struct core_vpu_sprite *spr) {
    int offs = (spr->b2 & VPU_SPR_XOFFSET) >> 4;
    return (offs - 8) * 8;
}

static inline int core_vpu__spr_yoffs(struct core_vpu_sprite *spr) {
    int offs = (spr->b2 & VPU_SPR_YOFFSET);
    return (offs - 8) * 8;
}

static inline int core_vpu__spr_tile(struct core_vpu_sprite *spr) {
    return spr->b3;
}


static void core_vpu__fetch_data(struct core_vpu *, int, int);
static struct rgba core_vpu__get_l2px(struct core_vpu *, int, int);
static struct rgba core_vpu__get_l1px(struct core_vpu *, int, int);
static struct rgba core_vpu__get_spx(struct core_vpu *, int, int, int);
static int core_vpu__get_l1t(struct core_vpu *vpu, int, int);
static int core_vpu__get_st(struct core_vpu *vpu, int, int, int);
static void core_vpu__write_px(struct core_vpu *, int, int, struct rgba);

/* 
 * Initialize the VPU state. This includes allocating the struct, and setting
 * the dependencies to the CPU, tile bank and framebuffer.
 */
int core_vpu_init(struct core_vpu **pvpu, struct core_cpu *cpu)
{
    struct core_vpu *vpu;

    *pvpu = malloc(sizeof(struct core_vpu));
    if(*pvpu == NULL) {
        LOGE("Could not allocate vpu core; exiting");
        return 0;
    }
    vpu = *pvpu;
    
    memset(vpu, 0, sizeof(struct core_vpu));
    vpu->cpu = cpu;
    vpu->mmu = cpu->mmu;
    
    vpu->tile_bank = vpu->mmu->tile_s;

    vpu->mem = malloc(3*1024);
    if(vpu->mem == NULL) {
        LOGE("Could not allocate video memory space; exiting");
        return 0;
    }
    vpu->layer1_tm = (uint8_t (*)[VPU_TILEMAP_SIZE])vpu->mem;
    vpu->layer2_tm = (uint8_t (*)[VPU_TILEMAP_SIZE])(vpu->mem + 0x480);
    vpu->pals = (uint8_t (*)[VPU_PALETTE_NUM*VPU_PALETTE_SZ])(vpu->mem + 0x900);
    vpu->spr_ctl = (uint8_t (*)[VPU_NUM_SPRITES*4])(vpu->mem + 0xa00);
    vpu->grp_pos = (uint8_t (*)[VPU_NUM_GROUPS*2])(vpu->mem + 0xb00);
    vpu->layers_pi = vpu->mem + 0xb80;
    vpu->spr_pi = vpu->mem + 0xb81;
    vpu->layer1_csx = vpu->mem + 0xb82;
    vpu->layer1_fsx = vpu->mem + 0xb83;
    vpu->layer1_csy = vpu->mem + 0xb84;
    vpu->layer1_fsy = vpu->mem + 0xb85;
    vpu->layer2_csx = vpu->mem + 0xb86;
    vpu->layer2_fsx = vpu->mem + 0xb87;
    vpu->layer2_csy = vpu->mem + 0xb88;
    vpu->layer2_fsy = vpu->mem + 0xb89;
    vpu->tile_s_bank = vpu->mem + 0xb90;

    vpu->rgba_fb = malloc(VPU_XRES * VPU_YRES * 4);

    vpu->sl__l1data_r = vpu->sl__l1data[0];
    vpu->sl__l2data_r = vpu->sl__l2data[0];
    vpu->sl__sdata_r = vpu->sl__sdata[0];

    vpu->sl__l1data_w = vpu->sl__l1data[1];
    vpu->sl__l2data_w = vpu->sl__l2data[1];
    vpu->sl__sdata_w = vpu->sl__sdata[1];

    return 1;
}


/* Free all memory allocated by the VPU. */
int core_vpu_destroy(struct core_vpu *vpu)
{
    free(vpu->rgba_fb);
    free(vpu->mem);
    free(vpu);
}

/* Copy the default palette into the VPU's private memory. */
int core_vpu_init_palette(struct core_vpu *vpu, uint8_t *palette)
{
    int i;
    uint8_t *p = palette;

    for(i = 0; i < 256; ++i) {
        pal_fixed[i].r = *p++;
        pal_fixed[i].g = *p++;
        pal_fixed[i].b = *p++;
        pal_fixed[i].a = 255; 
    }

    return 1;
}


/* Perform a per-frame state update of the VPU. */
void core_vpu_update(struct core_vpu *vpu)
{
    /* A new tile bank may have been switched in; point to the right one. */
    vpu->tile_bank = vpu->cpu->mmu->tile_s;
}


/* 
 * Write the tile layers and the sprites to the shared framebuffer.
 * It will then be presented at the next screen refresh.
 *
 * NOTE: This no longer accurately represents the function of the VPU.
 * It is not cycle- or scanline-accurate; instead, it can only provide
 * frame-level consistency of the picture, and does not track the memory
 * accesses or the internal temporaries of the VPU.
 *
 * Instead, simply run core_vpu_cycle() every cycle. It will update state and
 * do all the right things at the right time. It will also flip buffers.
 *
 * Use this only for a speed-hack.
 */
void core_vpu_write_fb(struct core_vpu *vpu)
{
    int l, tx, ty, s, z;
    int depth[VPU_NUM_SPR_LAYERS][VPU_NUM_SPRITES];
    int depth_num[VPU_NUM_SPR_LAYERS];

    /* Pin the framebuffer down for the update. */
    ui_lock_fb();
    
    /* First, wipe the previous framebuffer. */
    memset(vpu->rgba_fb, 0, VPU_XRES * VPU_YRES * sizeof(struct rgba)); 
    
    /* Next, render the tilemaps layers. */
    for(l = 0; l < 2; ++l) {
        for(ty = 0; ty < VPU_TILE_YRES; ++ty) {
            int scroll_y = (!l ? *vpu->layer1_csy : *vpu->layer2_csy) % 32;
            if(ty < scroll_y)
                continue;
            for(tx = 0; tx < VPU_TILE_XRES; ++tx) {
                uint8_t *tile;
                int index, x, y;
                int scroll_x = (!l ? *vpu->layer1_csx : *vpu->layer2_csx) % 28;
                int pi = !l ? core_vpu__pal_l1(vpu) : core_vpu__pal_l2(vpu);

                if(tx < scroll_x)
                    continue;

                index = !l ? (*vpu->layer1_tm)[ty * VPU_TILE_XRES] :
                                (*vpu->layer2_tm)[ty * VPU_TILE_XRES];
                tile = &vpu->tile_bank[index * VPU_TILE_SZ];
                for(y = 0; y < 8; ++y) {
                    uint8_t fsy = !l ? *vpu->layer1_fsy : *vpu->layer2_fsy;
                    uint8_t fsx = !l ? *vpu->layer1_fsx : *vpu->layer2_fsx;
                    /*
                     * Let's break this down:
                     * - the Y coordinate in the framebuffer is the number
                     *   of tiles downwards we are, TIMES the tile height,
                     *   PLUS the fine scroll, PLUS the current loop Y.
                     * - the X coordinate is simply current loop X PLUS the
                     *   fine scroll.
                     * The index is at Y*<row-width> + X.
                     */
                    int i = (ty*8 + y + fsy)*VPU_TILE_XRES*8;
                    for(x = 0; x < (8 >> 1); ++x) {
                        uint8_t p = tile[y * (8>>1) + x];
                        int hi, lo, j;
                        struct rgba rgb, *fbp;

                        j = 4*(i + 2*x + fsx + 8*tx);
                        fbp = (struct rgba *)&vpu->rgba_fb[j];

                        /* We use double X due to 4-bit palette indexes. */
                        hi = p >> 4;
                        lo = p & 0xf;

                        rgb = pal_fixed[(*vpu->pals)[pi*VPU_PALETTE_SZ + hi]];
                        *fbp = rgb;
                        rgb = pal_fixed[(*vpu->pals)[pi*VPU_PALETTE_SZ + lo]];
                        *(fbp + 1) = rgb;
                    }
                }
            }
        }
    }

    /* Finally, render the sprites. */
    memset(depth, 0, VPU_NUM_SPR_LAYERS * VPU_NUM_SPRITES * sizeof(int));
    memset(depth_num, 0, VPU_NUM_SPR_LAYERS * sizeof(int));
    /* First, sort them by depth/Z-index so they can be blitted in order. */
    for(s = 0; s < VPU_NUM_SPRITES; ++s) {
        struct core_vpu_sprite *spr = (void *)&(*vpu->spr_ctl)[s * 4];
        if(!core_vpu__spr_enabled(spr))
            continue;
        //LOGV("core.vpu: detected enabled sprite #%d", s);
        int d = core_vpu__spr_depth(spr);
        depth[d][depth_num[d]++] = s;
    }
    /* Now iterate over each "layer" of sprites. */
    for(z = VPU_NUM_SPR_LAYERS - 1; z >= 0; --z) {
        int ls;
        //LOGD("core.vpu: sprite layer %d", z);
        for(ls = 0; ls < depth_num[z]; ++ls) {
            struct core_vpu_sprite *spr;
            uint8_t *tile;
            int px, py;
            int startx, endx, dx;
            int starty, endy, dy;
            int h2, v2, hm, vm, g;
            int xoffs, yoffs;
            int t, pi;
            int x, y, tx, ty;

            s = depth[z][ls];
            spr = (void *)&vpu->spr_ctl[s * 4];

            g = core_vpu__spr_group(spr);
            xoffs = core_vpu__spr_xoffs(spr);
            yoffs = core_vpu__spr_yoffs(spr);
            t = core_vpu__spr_tile(spr);
            h2 = core_vpu__spr_hdouble(spr);
            v2 = core_vpu__spr_vdouble(spr);
            hm = core_vpu__spr_hmirror(spr);
            vm = core_vpu__spr_vmirror(spr);
            t = core_vpu__spr_tile(spr);
            pi = *vpu->spr_pi & VPU_SPRITE_PI;

            px = (*vpu->grp_pos)[g*2];
            py = (*vpu->grp_pos)[g*2 + 1];
       
#ifdef _DEBUG_VPU
            LOGD("core.vpu: rendering sprite:");
            LOGD("...group %d, tile %d, h2: %d, v2: %d, hm: %d, vm: %d",
                    g, t, h2, v2, hm, vm);
            LOGD("...palette %d, group.pos (%d, %d)  offs.(%d, %d)",
                    pi, px, py, xoffs, yoffs);
#endif

#define _MAX(x,y) ((x)>(y)?(x):(y))
#define _MIN(x,y) ((x)<(y)?(x):(y))
            startx = _MIN(_MAX(0, px + xoffs + (hm ? 7 + 7*h2 : 0)), 288);
            endx = _MIN(startx + (hm ? -(8 + 8*h2) : (8 + 8*h2)), 288);
            dx = hm ? (-1 - h2) : (1 + h2);
            starty = _MIN(_MAX(0, py + yoffs + (vm ? 7 + 8*v2 : 0)), 256);
            endy = _MIN(starty + (vm ? -(8 + 8*v2) : (8 + 8*v2)), 256);
            dy = vm ? -1 : 1; //(-1 - v2) : (1 + v2);
#undef _MIN
#undef _MAX
            tile = &vpu->tile_bank[t * VPU_TILE_SZ];
#ifdef _DEBUG_VPU
            LOGD("core.vpu: tile dump:");
            LOGD("...%02x %02x %02x %02x", 
                    tile[0], tile[1], tile[2], tile[3]);
            LOGD("...%02x %02x %02x %02x",
                    tile[4], tile[5], tile[6], tile[7]);
            LOGD("...%02x %02x %02x %02x",
                    tile[8], tile[9], tile[10], tile[11]);
            LOGD("...%02x %02x %02x %02x",
                    tile[12], tile[13], tile[14], tile[15]);
            LOGD("...%02x %02x %02x %02x",
                    tile[16], tile[17], tile[18], tile[19]);
            LOGD("...%02x %02x %02x %02x",
                    tile[20], tile[21], tile[22], tile[23]);
            LOGD("...%02x %02x %02x %02x",
                    tile[24], tile[25], tile[26], tile[27]);
            LOGD("...%02x %02x %02x %02x",
                    tile[28], tile[29], tile[30], tile[31]);
            
            LOGD("core.vpu: startx: %d, dx: %d, endx: %d", startx, dx, endx);
            LOGD("core.vpu: starty: %d, dy: %d, endy: %d", starty, dy, endy);
#endif

            for(y = starty, ty = 0; y != endy; ++ty) {
                int vr;
                for(vr = 0; vr < v2+1; ++vr, y += dy) {
                    for(x = startx, tx = 0; tx < 4; x += 2*dx, ++tx) {
                        uint8_t p;
                        uint8_t hi, lo;
                        struct rgba rgb, *fbp;

                        p = tile[ty * (8>>1) + tx];

                        hi = p >> 4;
                        lo = p & 0xf;
                        if(hm) {
                            uint8_t temp = hi;
                            hi = lo;
                            lo = temp;
                        }

                        fbp = (struct rgba *)&vpu->rgba_fb[(y*VPU_XRES + x) * 4];

                        rgb = pal_fixed[(*vpu->pals)[pi*VPU_PALETTE_SZ + hi]];
                        *fbp++ = rgb;
                        if(h2) *fbp++ = rgb;
                        rgb = pal_fixed[(*vpu->pals)[pi*VPU_PALETTE_SZ + lo]];
                        *fbp++ = rgb;
                        if(h2) *fbp++ = rgb;
                    }
                }
            }
        }
    }

    /* The framebuffer is now ready for use by the UI thread. */
    ui_unlock_fb();
}


/* 
 * Re-entrant function for VPU emulation.
 *
 * Tile data for the layers' tiles and sprites on the current scanline
 * are loaded into buffers for drawing, along with the current palettes.
 *
 * The VPU then renders them following their depth and mirroring/doubling.
 */
void core_vpu_cycle(struct core_vpu *vpu, int total_cycles)
{
    uint8_t *temp;
    static int scanline = 0;
    int c = total_cycles % VPU_XRES_CYCLES;

    /* First, update state if necessary. */
    core_vpu_update(vpu);

    if(scanline == 12 && c == 0)
        core_vpu_end_vblank(vpu);

    /* Scanlines 0-11 and 240 - 261 are V-BLANK lines. We do nothing there.
     * Otherwise, we are in pre-render or render lines. */
    else if(scanline >= 15 && scanline < 240) {

        core_vpu__fetch_data(vpu, scanline, c);

        if (scanline >= 16) {
            /* Cycles 0-24: H-SYNC. */
            /* Cycles 25-64: Back porch and colorburst. */
    
            /* Cycles 65-320: Pixel data! */
            if(c >= 65 && c < 321) {
                struct rgba out, l1, l2, s[VPU_NUM_SPRITES] = {0};
                int i, l1t, st[VPU_NUM_SPRITES] = {0};
                int x = c - 65;
                
                /* Get RGB and transparency data for each layer and sprite's
                 * pixel. */
                out = core_vpu__get_l2px(vpu, scanline, c);
                if (!core_vpu__get_l1t(vpu, scanline, c))
                    out = core_vpu__get_l1px(vpu, scanline, c);
                for(i = 0; i < VPU_NUM_SPRITES; ++i) {
                    struct core_vpu_sprite *spr = (void *)&(*vpu->spr_ctl)[i*4];
                    if (!core_vpu__spr_enabled(spr))
                        continue;
                    int hdouble = core_vpu__spr_hdouble(spr);
                    int vdouble = core_vpu__spr_vdouble(spr);
                    uint8_t grp = core_vpu__spr_group(spr);
                    int startx = (*vpu->grp_pos)[grp*2] + core_vpu__spr_xoffs(spr);
                    int endx = startx + (hdouble ? 16 : 8);
                    int starty = (*vpu->grp_pos)[grp*2 + 1] + core_vpu__spr_yoffs(spr);
                    int endy = starty + (vdouble ? 16 : 8);
                    if((x >= startx) &&
                       (x < endx) &&
                       ((scanline-16) >= starty) &&
                       ((scanline-16) < endy)) {
                        if (!core_vpu__get_st(vpu, scanline, c, i))
                            out = core_vpu__get_spx(vpu, scanline, c, i);
                    }
                }
    
                /* Finally, output the pixel to the framebuffer. */
                core_vpu__write_px(vpu, scanline, c, out);
            }
        }

    } else if(scanline == 240 && c == 0) {
        core_vpu_begin_vblank(vpu);
    }

    /* The last cycle of the scanline is a good time to increment
     * the scanline counter, and wrap it if necessary! */
    if(c == 340) {
        scanline = (scanline + 1) % VPU_YRES_SCANLINES;
        /* XXX: this might be a good place to implement the double
         * buffering's framebuffer swap. */
        if(scanline == 0) {
            LOGV("core.vpu: frame end");
        }

        /* Swap the scanline temps. */
        temp = vpu->sl__l1data_r;
        vpu->sl__l1data_r = vpu->sl__l1data_w;
        vpu->sl__l1data_w = temp;
        temp = vpu->sl__l2data_r;
        vpu->sl__l2data_r = vpu->sl__l2data_w;
        vpu->sl__l2data_w = temp;
        temp = vpu->sl__sdata_r;
        vpu->sl__sdata_r = vpu->sl__sdata_w;
        vpu->sl__sdata_w = temp;
    }
}


/* Makes the correct memory accesses for a given scanline and cycle. */
static void core_vpu__fetch_data(struct core_vpu *vpu, int scanline, int c)
{
    unsigned int a = c - 25;
    /* We are writing to the next scanline, not the current one, so add 1. */
    unsigned int y = scanline - (16 - 1);

    /* The VPU is idle during the vertical blanking interval. */
    if(scanline < (16 - 1) || scanline >= (240 - 1))
        return;

    /* The VPU is also idle during every scanline during its horizontal sync
     * period. */
    if(c < 25)
        return;

    /* For "regular" scanlines, the following reads are performed:
     * - layer 1 tile data (4*32 = 128 bytes; 64 reads)
     * - layer 2 tile data (4*32 = 128 bytes; 64 reads)
     * - sprite 0-63 tile data (4*64 = 256; 128 reads)
     * This adds up to 256 reads per scanline.
     */
    if(a < 64) {
        /* At cycle 0, there is no previous read request to read back. */
        if(a > 0)
            *(uint16_t *)&vpu->sl__l1data_w[(a - 1)*2] = core_mmu_rw_fetch_vpu(vpu->mmu);
        /* Fetch the tile data for tile[y][x], where:
         * - y = (scanline - 12) / 8 (tiles are 8 scanlines tall)
         * - x = (c - 25) / (8/2) (tiles are 4 BYTES wide) */
        core_mmu_rw_send_vpu(vpu->mmu,
                (VPU_A_TILE_BANK + (a % 2)*2 + (y % 8)*4 +
                 (*vpu->layer1_tm)[(y >> 3)*VPU_TILE_XRES_FULL + (a >> 2)]));
    } else if(a < 128) {
        /* At cycle 64, read back last read request for layer 1. */
        if(a == 64)
            *(uint16_t *)&vpu->sl__l1data_w[63 * 2] = core_mmu_rw_fetch_vpu(vpu->mmu);
        else
            *(uint16_t *)&vpu->sl__l2data_w[(a - 1 - 64)*2] = core_mmu_rw_fetch_vpu(vpu->mmu);
        /* Fetch the tile data for tile[y][x], where:
         * - y = (scanline - 12) / 8 (tiles are 8 scanlines tall)
         * - x = (c - 25) / (8/2) (tiles are 4 BYTES wide) */
        core_mmu_rw_send_vpu(vpu->mmu,
                (VPU_A_TILE_BANK + (a % 2)*2 + (y % 8)*4 +
                 (*vpu->layer2_tm)[(y >> 3)*VPU_TILE_XRES_FULL + ((a - 64) >> 2)]));
    } else if(a < 256) {
        unsigned int ac = a - 128;
        unsigned int cs = ac / 2;                       // Current sprite
        int vd = (*vpu->spr_ctl)[cs*4] & VPU_SPR_VDOUBLE;
        unsigned int sg = (*vpu->spr_ctl)[cs*4 + 1];    // Sprite group
        unsigned int gy0 = (*vpu->grp_pos)[sg*2 + 1];   // Group start y
        unsigned int sy0 = gy0 + (((*vpu->spr_ctl)[cs*4 + 2] & 0x0f) - 8)*8; // Sprite start y
        unsigned int csy = (y - sy0) >> vd;             // Current sprite y
        /* At cycle 128, read back last read request for layer 2. */
        if(a == 128)
            *(uint16_t *)&vpu->sl__l2data_w[63 * 2] = core_mmu_rw_fetch_vpu(vpu->mmu);
        else
            *(uint16_t *)&vpu->sl__sdata_w[(ac - 1)*2] = core_mmu_rw_fetch_vpu(vpu->mmu);
        /* Fetch the tile data for sprite i, where:
         * - i = (scanline - 12) / (8/2) */
        core_mmu_rw_send_vpu(vpu->mmu,
                 VPU_A_TILE_BANK +                          // Tile bank base
                 ((*vpu->spr_ctl)[cs*4 + 3] * VPU_TILE_SZ) +  // Offset to sprite tile
                 (csy * 4) +                                // Offset due to scanline
                 ((a % 2) * 2));                            // Offset due to sl. cycle
    } else if(a == 256) {
        /* Fetch the last word of sprite data. */
        *(uint16_t *)&vpu->sl__sdata_w[127 * 2] = core_mmu_rw_fetch_vpu(vpu->mmu);
    }
}


/* Return layer 2's tilemap pixel at the current scanline and cycle. */
static struct rgba core_vpu__get_l2px(struct core_vpu *vpu, int scanline, int c)
{
    int x = (c - 65) & 255;
    int tx = x / 2;
    uint8_t e = vpu->sl__l2data_r[tx];
    e = (c & 1) ? (e >> 4) : (e & 0xf);

    return pal_fixed[(*vpu->pals)[e]];
    //return vpu->sl__l2pal[e];
}


/* Return layer 1's tilemap pixel at the current scanline and cycle. */
static struct rgba core_vpu__get_l1px(struct core_vpu *vpu, int scanline, int c)
{
    int x = (c - 65) & 255;
    int tx = x / 2;
    uint8_t e = vpu->sl__l1data_r[tx];
    e = (c & 1) ? (e >> 4) : (e & 0xf);

    return pal_fixed[(*vpu->pals)[e]];
    //return vpu->sl__l1pal[e];
}


/* Return sprite i's pixel at the current scanline and cycle. */
static struct rgba core_vpu__get_spx(struct core_vpu *vpu, int scanline, int c,
                                    int i)
{
    struct rgba dummy = { 0 };
    int x = (c - 65) & 255;
    uint8_t grp = (*vpu->spr_ctl)[i*4 + 1];
    int tx = (x - (*vpu->grp_pos)[grp*2]) / 2;
    if(tx < 0)
        return dummy;
    int h2 = !!((*vpu->spr_ctl)[i*4] & VPU_SPR_HDOUBLE);
    uint8_t e = vpu->sl__sdata_r[i*4 + (tx >> h2)];
    int lp = h2 ? !(x & 2) : (c & 1);
    e = lp ? (e >> 4) : (e & 0xf);

    return pal_fixed[(*vpu->pals)[e]];
    //return vpu->sl__spal[e];
}


/* Return whether layer 1's tilemap pixel at the current scanline and cycle is
 * transparent. */
static int core_vpu__get_l1t(struct core_vpu *vpu, int scanline, int c)
{
    int x = (c - 65) & 255;
    int tx = x / 2;
    uint8_t e = vpu->sl__l1data_r[tx];
    e = (c & 1) ? (e >> 4) : (e & 0xf);

    return !e;
}


/* Return whether sprite i's pixel at the current scanline and cycle is
 * transparent. */
static int core_vpu__get_st(struct core_vpu *vpu, int scanline, int c, int i)
{
    int x = (c - 65) & 255;
    uint8_t grp = (*vpu->spr_ctl)[i*4 + 1];
    int tx = (x - (*vpu->grp_pos)[grp*2]) / 2;
    if(tx < 0)
        return 1;
    int h2 = !!((*vpu->spr_ctl)[i*4] & VPU_SPR_HDOUBLE);
    uint8_t e = vpu->sl__sdata_r[i*4 + (tx >> h2)];
    int lp = h2 ? !(x & 2) : (c & 1);
    e = lp ? (e >> 4) : (e & 0xf);

    return !e;
}


/* Write the given pixel to the virtual framebuffer at the position
 * corresponding to the current scanline's c-th cycle. */
static void core_vpu__write_px(struct core_vpu *vpu, int scanline, int c,
                                struct rgba pixel)
{
    int x = c - 65;
    int y = scanline - 16;
    struct rgba *fb = (struct rgba *) vpu->rgba_fb;

    fb[y * 256 + x] = pixel;
}


/* Signal the start of the VBlank period, by firing the video interrupt. */
void core_vpu_begin_vblank(struct core_vpu *vpu)
{
    vpu->cpu->interrupt = INT_VIDEO_IRQ;
    vpu->vblank = 1;
    ui_lock_fb();
    {
        void *uifb =  ui_get_fb();
        memcpy(uifb, vpu->rgba_fb, VPU_XRES * VPU_YRES * 4);
    }
    ui_unlock_fb();   
}

/* Signal the end of the VBlank period. */
void core_vpu_end_vblank(struct core_vpu *vpu)
{
    vpu->vblank = 0;
}


uint8_t core_vpu_readb(struct core_vpu *vpu, uint16_t a)
{
    if(!vpu->vblank) {
        LOGV("core.vpu: read denied: vblank = 0");
        return 0;
    }
#ifdef _DEBUG_MEMORY
    LOGD("core.vpu: read byte @ $%04x", a);
#endif
    return vpu->mem[a - 0xe000];
}

void core_vpu_writeb(struct core_vpu *vpu, uint16_t a, uint8_t v)
{
    if(!vpu->vblank) {
        LOGV("core.vpu: write denied: vblank = 0");
        return;
    }
#ifdef _DEBUG_MEMORY
    LOGD("core.vpu: wrote %02x @ $%04x", v, a);
#endif
    vpu->mem[a - 0xe000] = v;
}

uint16_t core_vpu_readw(struct core_vpu *vpu, uint16_t a)
{
    uint8_t hi, lo;
    hi = core_vpu_readb(vpu, a);
    lo = core_vpu_readb(vpu, a+1);
    return hi << 8 | lo;
}

void core_vpu_writew(struct core_vpu *vpu, uint16_t a, uint16_t v)
{
    core_vpu_writeb(vpu, a, v & 0xff);
    core_vpu_writeb(vpu, a+1, v >> 8);
}

#undef _DEBUG_VPU
#undef _DEBUG_MEMORY

