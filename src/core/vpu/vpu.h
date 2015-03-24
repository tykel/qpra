/*
 * core/vpu/vpu.h -- Emulator VPU functions (header).
 *
 * Defines the structures used to represent the Video Processing Unit.
 * Declares all the VPU functions.
 *
 */

#include <stdint.h>

#define VPU_TILEMAP_SIZE    0x480
#define VPU_NUM_SPRITES     64
#define VPU_NUM_GROUPS      64
#define VPU_PALETTE_SZ      16
#define VPU_NUM_PALETTES    16
#define VPU_XRES            256
#define VPU_YRES            224
#define VPU_TILE_XRES       32
#define VPU_TILE_YRES       28

struct core_cpu;
struct core_mmu;

/* Structure used as an overlay over framebuffer. */
#pragma pack(push, 1)
struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};
#pragma pack(pop)

/* VPU state structure. */
struct core_vpu {
    struct core_cpu *cpu;
    struct core_mmu *mmu;

    uint8_t *tile_bank;
    uint8_t layer1_tm[VPU_TILEMAP_SIZE];
    uint8_t layer2_tm[VPU_TILEMAP_SIZE];
    
    uint8_t palette[VPU_NUM_PALETTES * VPU_PALETTE_SZ];
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
int core_vpu_destroy(struct core_vpu *);

void core_vpu_update(struct core_vpu *);
void core_vpu_write_fb(struct core_vpu *);

uint8_t core_vpu_readb(struct core_vpu *, uint16_t);
void core_vpu_writeb(struct core_vpu *, uint16_t, uint8_t);
uint16_t core_vpu_readw(struct core_vpu *, uint16_t);
void core_vpu_writew(struct core_vpu *, uint16_t, uint16_t);

