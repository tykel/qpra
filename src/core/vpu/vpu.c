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
#include "log.h"

/* Function provided by libui which gives us a pointer to the FB texture. */
void *ui_get_fb();


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
    /* XXX: Fix this. */
    vpu->tile_bank = cpu->mmu->tile_s;
    vpu->rgba_fb = ui_get_fb();
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
    int l, tx, ty, s;

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

                if(tx < scroll_x)
                    continue;

                index = !l ? vpu->layer1_tm[ty * VPU_TILE_XRES] :
                                vpu->layer2_tm[ty * VPU_TILE_XRES];
                tile = &vpu->tile_bank[index * (8 * (8>>1))];
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
                    for(x = 0; x < (8 >> 1); ++y) {
                        uint8_t p = tile[y*8 + x];
                        int hi, lo;
                        
                        /* We use double X due to 4-bit palette indexes. */
                        hi = p >> 4;
                        vpu->rgba_fb[i + 2*(x+fsx)] = hi;

                        lo = p & 0xf;
                        vpu->rgba_fb[i + 2*(x+fsx) +1] = lo;
                    }
                }
            }
        }
    }

    /* Finally, render the sprites. */
    /* TODO. */
}
