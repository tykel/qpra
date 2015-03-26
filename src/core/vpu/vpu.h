/*
 * core/vpu/vpu.h -- Emulator VPU functions (header).
 *
 * Defines the structures used to represent the Video Processing Unit.
 * Declares all the VPU functions.
 *
 */

#ifndef QPRA_CORE_VPU_H
#define QPRA_CORE_VPU_H

#include <stdint.h>

#define VPU_TILEMAP_SIZE    0x480
#define VPU_TILE_SZ         32
#define VPU_NUM_SPRITES     64
#define VPU_NUM_GROUPS      64
#define VPU_FIX_PALETTE_SZ  1024
#define VPU_PALETTE_NUM     16
#define VPU_PALETTE_SZ      16
#define VPU_XRES            256
#define VPU_YRES            224
#define VPU_TILE_XRES       32
#define VPU_TILE_YRES       28
#define VPU_NUM_SPR_LAYERS  8

#define VPU_LAYER1_PI       0b11110000
#define VPU_LAYER2_PI       0b00001111
#define VPU_SPRITE_PI       0b00001111

#define VPU_SPR_ENABLE      0b10000000
#define VPU_SPR_DEPTH       0b01110000
#define VPU_SPR_HMIRROR     0b00001000
#define VPU_SPR_VMIRROR     0b00000010
#define VPU_SPR_HDOUBLE     0b00000100
#define VPU_SPR_VDOUBLE     0b00000001

#define VPU_SPR_GROUP       0b00111111

#define VPU_SPR_XOFFSET     0b11110000
#define VPU_SPR_YOFFSET     0b00001111

#define VPU_A_L1TM          0xe000
#define VPU_A_L1TM_END      0xe47f
#define VPU_A_L2TM          0xe480
#define VPU_A_L2TM_END      0xe8ff
#define VPU_A_PALS          0xe900
#define VPU_A_PALS_END      0xe9ff
#define VPU_A_SPRCTL        0xea00
#define VPU_A_SPRCTL_END    0xeaff
#define VPU_A_SPRCOORD      0xeb00
#define VPU_A_SPRCOORD_END  0xeb7f
#define VPU_A_L12_PAL       0xeb80
#define VPU_A_SPR_PAL       0xeb81
#define VPU_A_L1_SCR        0xeb82
#define VPU_A_L1_SCR_END    0xeb85
#define VPU_A_L2_SCR        0xeb86
#define VPU_A_L2_SCR_END    0xeb89
#define VPU_A_TILE_B_SELECT 0xeb90

struct core_cpu;
struct core_mmu;

/* Structure used as an overlay over framebuffer. */
struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

/* Structure to map over sprites. */
struct core_vpu_sprite {
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
};

/* VPU state structure. */
struct core_vpu {
    struct core_cpu *cpu;
    struct core_mmu *mmu;

    uint8_t *tile_bank;
    uint8_t layer1_tm[VPU_TILEMAP_SIZE];
    uint8_t layer2_tm[VPU_TILEMAP_SIZE];
    
    uint8_t pals[VPU_PALETTE_NUM * VPU_PALETTE_SZ];
    uint8_t spr_ctl[VPU_NUM_SPRITES * 4];
    uint8_t grp_pos[VPU_NUM_GROUPS * 2];
    
    uint8_t layers_pi;
    uint8_t spr_pi;
    
    uint8_t layer1_csx;
    uint8_t layer1_fsx;
    uint8_t layer1_csy;
    uint8_t layer1_fsy;

    uint8_t layer2_csx;
    uint8_t layer2_fsx;
    uint8_t layer2_csy;
    uint8_t layer2_fsy;

    uint8_t *rgba_fb;
};

/* Function declarations. */
int core_vpu_init(struct core_vpu **, struct core_cpu *);
int core_vpu_init_palette(struct core_vpu *, uint8_t *);
int core_vpu_destroy(struct core_vpu *);

void core_vpu_update(struct core_vpu *);
void core_vpu_write_fb(struct core_vpu *);

uint8_t core_vpu_readb(struct core_vpu *, uint16_t);
void core_vpu_writeb(struct core_vpu *, uint16_t, uint8_t);
uint16_t core_vpu_readw(struct core_vpu *, uint16_t);
void core_vpu_writew(struct core_vpu *, uint16_t, uint16_t);

#endif

