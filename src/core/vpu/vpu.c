/*
 * core/vpu/vpu.c -- Emulator VPU functions.
 *
 * The functions used by the Video Processing Unit.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "core/vpu/vpu.h"
#include "core/cpu/cpu.h"
#include "core/mmu/mmu.h"
#include "ui/ui.h"
#include "log.h"

struct rgba pal_fixed[VPU_FIX_PALETTE_SZ];


static inline int core_vpu__pal_l1(struct core_vpu *vpu) {
    return (vpu->layers_pi & VPU_LAYER1_PI) >> 4;
}

static inline int core_vpu__pal_l2(struct core_vpu *vpu) {
    return (vpu->layers_pi & VPU_LAYER2_PI);
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
    return (spr->b2 & VPU_SPR_XOFFSET) >> 4;
}

static inline int core_vpu__spr_yoffs(struct core_vpu_sprite *spr) {
    return (spr->b2 & VPU_SPR_YOFFSET);
}

static inline int core_vpu__spr_tile(struct core_vpu_sprite *spr) {
    return spr->b3;
}


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
    /* XXX: Fix this. */
    vpu->tile_bank = vpu->mmu->tile_s;
    vpu->rgba_fb = ui_get_fb();
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


/* Perform a per-cycle update of the VPU. */
void core_vpu_update(struct core_vpu *vpu)
{
    /* XXX: Fix this. */
    vpu->tile_bank = vpu->cpu->mmu->tile_s;
}


/* 
 * Write the tile layers and the sprites to the shared framebuffer.
 * It will then be presented at the next screen refresh.
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
            int scroll_y = (!l ? vpu->layer1_csy : vpu->layer2_csy) % 32;
            if(ty < scroll_y)
                continue;
            for(tx = 0; tx < VPU_TILE_XRES; ++tx) {
                uint8_t *tile;
                int index, x, y;
                int scroll_x = (!l ? vpu->layer1_csx : vpu->layer2_csx) % 28;
                int pi = !l ? core_vpu__pal_l1(vpu) : core_vpu__pal_l2(vpu);

                if(tx < scroll_x)
                    continue;

                index = !l ? vpu->layer1_tm[ty * VPU_TILE_XRES] :
                                vpu->layer2_tm[ty * VPU_TILE_XRES];
                if(index)
                    LOGD("core.vpu: found non-0 index %d at T(%d,%d)",
                         index, tx, ty);
                tile = &vpu->tile_bank[index * VPU_TILE_SZ];
                for(y = 0; y < 8; ++y) {
                    uint8_t fsy = !l ? vpu->layer1_fsy : vpu->layer2_fsy;
                    uint8_t fsx = !l ? vpu->layer1_fsx : vpu->layer2_fsx;
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
                        int hi, lo;
                        struct rgba rgb, *fbp;
                        
                        fbp = (struct rgba *)&vpu->rgba_fb[(i + 2*x + fsx)*4];

                        /* We use double X due to 4-bit palette indexes. */
                        hi = p >> 4;
                        lo = p & 0xf;

                        rgb = pal_fixed[vpu->pals[pi*VPU_PALETTE_SZ + hi]];
                        *fbp = rgb;
                        rgb = pal_fixed[vpu->pals[pi*VPU_PALETTE_SZ + lo]];
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
        struct core_vpu_sprite *spr = (void *)&vpu->spr_ctl[s * 4];
        if(!core_vpu__spr_enabled(spr))
            continue;
        int d = core_vpu__spr_depth(spr);
        depth[d][depth_num[d]++] = s;
    }
    /* Now iterate over each "layer" of sprites. */
    for(z = 0; z < VPU_NUM_SPR_LAYERS; ++z) {
        int ls;
        for(ls = 0; ls < depth_num[z]; ++ls) {
            struct core_vpu_sprite *spr;
            uint8_t *tile;
            int px, py;
            int startx, endx, dx;
            int starty, endy, dy;
            int h2, v2, hm, vm, g;
            int xoffs, yoffs;
            int t, pi;
            int x, y;

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
            pi = vpu->spr_pi & VPU_SPRITE_PI;

            px = vpu->grp_pos[g*2];
            py = vpu->grp_pos[g*2 + 1];

#define _MAX(x,y) ((x)>(y)?(x):(y))
            startx = _MAX(0, px + (xoffs - 8) + (hm ? 7 + 8*h2 : 0));
            endx = startx + (hm ? -(8 + 8*h2) : (8 + 8*h2));
            dx = hm ? (-1 - h2) : (1 + h2);
            starty = _MAX(0, py + (yoffs - 8) + (vm ? 7 + 8*v2 : 0));
            endy = starty + (vm ? -(8 + 8*v2) : (8 + 8*v2));
            dy = hm ? (-1 - v2) : (1 + v2);
#undef _MAX
            tile = &vpu->tile_bank[t * VPU_TILE_SZ];

            for(y = starty; y != endy; ++y) {
                for(x = startx; x != endx; ++x) {
                    uint8_t p;
                    uint8_t hi, lo;
                    struct rgba rgb, *fbp;

                    p = tile[y * (8>>1) + x];

                    hi = p >> 4;
                    lo = p & 0xf;
                    fbp = (struct rgba *)&vpu->rgba_fb[(y*VPU_XRES + x) * 4];

                    rgb = pal_fixed[vpu->pals[pi*VPU_PALETTE_SZ + hi]];
                    *fbp = rgb;
                    rgb = pal_fixed[vpu->pals[pi*VPU_PALETTE_SZ + lo]];
                    *fbp = rgb;
                }
            }
        }
    }

    /* The framebuffer is now ready for use by the UI thread. */
    ui_unlock_fb();
}