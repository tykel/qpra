/*
 * core/mmu/mmu.c -- Emulator MMU functions.
 *
 * The functions used by the Memory Management Unit.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "core/core.h"
#include "core/mmu/mmu.h"
#include "core/cpu/cpu.h"
#include "core/cpu/hrc.h"
#include "core/vpu/vpu.h"
#include "log.h"

/* Static memory banks. */
static uint8_t *rom_f;
static uint8_t **rom_s;
static uint8_t *ram_f;
static uint8_t **ram_s;
static uint8_t **tile_s;
static uint8_t **dpcm_s;
static uint8_t *cart_f;
static uint8_t *fixed0_f;
static uint8_t *fixed1_f;

/* Private functions. */
static uint8_t core_mmu_readb(struct core_mmu *, uint16_t);
static void core_mmu_writeb(struct core_mmu *, uint16_t, uint8_t);
static uint16_t core_mmu_readw(struct core_mmu *, uint16_t);
static void core_mmu_writew(struct core_mmu *, uint16_t, uint16_t);

/*
 * Initialize the MMU.
 * Allocates memory for the core_mmu structure, and for each of the memory
 * buffers it owns.
 * Sets up callbacks for I/O which redirects to another system component.
 */
int core_mmu_init(struct core_mmu **pmmu, struct core_mmu_params *params,
        struct core_temp_banks *banks)
{
    int i;
    struct core_mmu *mmu;
   
    /* First, allocate the MMU structure. */
    *pmmu = NULL;
    *pmmu = malloc(sizeof(struct core_mmu));
    if(*pmmu == NULL) {
        LOGE("Could not allocate mmu core; exiting");
        return 0;
    }
    mmu = *pmmu;
    
    /* Allocate the two fixed banks. */
    rom_f = banks->rom_f; 
    mmu->rom_f = rom_f;

    ram_f = banks->ram_f ? banks->ram_f : calloc(8*1024, sizeof(uint8_t));
    mmu->ram_f = ram_f;

    /* Allocate the cart permanent storage. */
    cart_f = calloc(256, sizeof(uint8_t));
    if(cart_f == NULL)
        goto l_malloc_error;
    mmu->cart_f = cart_f;

    /* Allocate the two banks for misc. use at address space end. */
    fixed0_f = calloc(6*256, sizeof(uint8_t));
    fixed1_f = calloc(256, sizeof(uint8_t));
    if(fixed0_f == NULL || fixed1_f == NULL)
        goto l_malloc_error;
    mmu->fixed0_f = fixed0_f;
    mmu->fixed1_f = fixed1_f;

    /* Clear the interrupt vector. */
    memset(mmu->intvec, 0, sizeof(mmu->intvec));

    /* Allocate the switchable ROM banks. */
    if(params->rom_banks == 0) {
        LOGE("Requested 0 swappabled ROM banks; minimum is 1");
        return 0;
    }
    mmu->rom_s_total = params->rom_banks;
    rom_s = calloc(params->rom_banks, sizeof(uint8_t *));
    if(rom_s == NULL)
        goto l_malloc_error;
    for(i = 0; i < params->rom_banks; ++i) {
        rom_s[i] = banks->rom_s[i] ?
            banks->ram_f :
            calloc(8*1024, sizeof(uint8_t)); 
    }
    mmu->rom_s = rom_s[0]; 

    /* Allocate the switchable RAM banks. */
    if(params->ram_banks == 0) {
        LOGE("Requested 0 swappable RAM banks; minimum is 1");
        return 0;
    }
    mmu->ram_s_total = params->ram_banks;
    ram_s = calloc(params->ram_banks, sizeof(uint8_t *));
    if(ram_s == NULL)
        goto l_malloc_error;
    for(i = 0; i < params->ram_banks; ++i) {
        ram_s[i] = banks->ram_s[i] ?
            banks->ram_s[i] :
            calloc(8*1024, sizeof(uint8_t)); 
    }
    mmu->ram_s = ram_s[0];

    /* Allocate the switchable tile ROM banks. */
    if(params->tile_banks == 0) {
        LOGE("Requested 0 tile banks; minimum is 1");
        return 0;
    }
    mmu->tile_s_total = params->tile_banks;
    tile_s = calloc(params->tile_banks, sizeof(uint8_t *));
    if(tile_s == NULL)
        goto l_malloc_error;
    for(i = 0; i < params->tile_banks; ++i) {
        tile_s[i] = banks->tile_s[i] ?
            banks->tile_s[i] :
            calloc(8*1024, sizeof(uint8_t)); 
    }
    mmu->tile_s = tile_s[0];

    /* Allocate the switchable DPCM ROM banks. */
    if(params->dpcm_banks == 0) {
        LOGE("Requested 0 DPCM banks; minimum is 1");
        return 0;
    }
    mmu->dpcm_s_total = params->dpcm_banks;
    dpcm_s = calloc(params->dpcm_banks, sizeof(uint8_t *));
    if(dpcm_s == NULL)
        goto l_malloc_error;
    for(i = 0; i < params->dpcm_banks; ++i) {
        dpcm_s[i] = banks->dpcm_s[i] ?
            banks->dpcm_s :
            calloc(2*1024, sizeof(uint8_t)); 
    }
    mmu->dpcm_s = dpcm_s[0];
   
    /* Everything was allocated properly, phew. */
    LOGD("Allocated: %hhu ROM bank%s, %hhu RAM bank%s, %hhu tile ROM bank%s,"
         " %hhu DPCM ROM bank%s",
         params->rom_banks, params->rom_banks > 1 ? "s" : "",
         params->ram_banks, params->ram_banks > 1 ? "s" : "",
         params->tile_banks, params->tile_banks > 1 ? "s" : "",
         params->dpcm_banks, params->dpcm_banks > 1 ? "s" : "");

    return 1;

l_malloc_error:
    LOGE("Failed to allocate memory for banks");
    return 0;
}


/* 
 * Set the internal pointer to the core_cpu structure.
 * This is separate from the _init function since the cpu init function
 * requires a pointer to a valid core_mmu structure. Hence, MMU setup is done
 * in two steps.
 */
int core_mmu_cpu(struct core_mmu *mmu, struct core_cpu *cpu)
{
    if(mmu == NULL) {
        LOGE("Attempted to set cpu for null mmu");
        return 0;
    }

    mmu->cpu = cpu;
    return 1;
}

/* Similar rationale to the above, except for the VPU state. */
int core_mmu_vpu(struct core_mmu *mmu, struct core_vpu *vpu)
{
    if(vpu == NULL) {
        LOGE("Attempted to set vpu for null mmu");
        return 0;
    }

    mmu->vpu = vpu;
    return 1;
}


/* 
 * Destroy the MMU state.
 * Frees all the memory buffers it owns.
 */
int core_mmu_destroy(struct core_mmu *mmu)
{
    int i;

    free(rom_f);
    mmu->rom_f = rom_f = NULL;
    free(ram_f);
    mmu->ram_f = ram_f = NULL;
    free(cart_f);
    mmu->cart_f = cart_f = NULL;
    free(fixed0_f);
    mmu->fixed0_f = fixed0_f = NULL;
    free(fixed1_f);
    mmu->fixed1_f = fixed1_f = NULL;

    for(i = 0; i < mmu->rom_s_total; ++i)
        free(rom_s[i]);
    free(rom_s);
    mmu->rom_s = NULL, rom_s = NULL;
    
    for(i = 0; i < mmu->ram_s_total; ++i)
        free(ram_s[i]);
    free(ram_s);
    mmu->ram_s = NULL, ram_s = NULL;
    
    for(i = 0; i < mmu->tile_s_total; ++i)
        free(tile_s[i]);
    free(tile_s);
    mmu->tile_s = NULL, tile_s = NULL;

    for(i = 0; i < mmu->dpcm_s_total; ++i)
        free(dpcm_s[i]);
    free(dpcm_s);
    mmu->dpcm_s = NULL, dpcm_s = NULL;
    
    free(mmu);

    return 1;
}


/* 
 * Select the bank index for a specific memory bank.
 * Causes the correct bank to be switched in, and the previous one switched
 * out.
 */
int core_mmu_bank_select(struct core_mmu *mmu, enum core_mmu_bank bank,
                         uint8_t index)
{
    switch(bank) {
        case B_ROM_SWAP:
            mmu->rom_s_bank = index;
            mmu->rom_s = rom_s[index];
            break;
        case B_RAM_SWAP:
            mmu->ram_s_bank = index;
            mmu->ram_s = ram_s[index];
            break;
        case B_TILE_SWAP:
            mmu->tile_bank = index;
            mmu->tile_s = tile_s[index];
            break;
        case B_DPCM_SWAP:
            mmu->dpcm_bank = index;
            mmu->dpcm_s = dpcm_s[index];
            break;
    }
    return 1;
}


/* CPU memory operations. */
/* Place a Read-Byte request on the bus. */
int core_mmu_rb_send_cpu(struct core_mmu *mmu, uint16_t a)
{
    mmu->pending_cpu = MMU_READ;
    mmu->a_cpu = a;
    mmu->vsz_cpu = 1;
    return 1;
}


/* Return the result of the Read-Byte request from the MDR. */
uint8_t core_mmu_rb_fetch_cpu(struct core_mmu *mmu)
{
    return (uint8_t) mmu->v_cpu;
}


/* Place a Write-Byte request on the bus. */
int core_mmu_wb_send_cpu(struct core_mmu *mmu, uint16_t a, uint8_t v)
{
    mmu->pending_cpu = MMU_WRITE;
    mmu->a_cpu = a;
    mmu->v_cpu = v;
    mmu->vsz_cpu = 1;
    return 1;
}


/* Place a Read-Word request on the bus. */
int core_mmu_rw_send_cpu(struct core_mmu *mmu, uint16_t a)
{
    mmu->pending_cpu = MMU_READ;
    mmu->a_cpu = a;
    mmu->vsz_cpu = 2;
    return 1;
}


/* Return the result of the Read-Word request from the MDR. */
uint16_t core_mmu_rw_fetch_cpu(struct core_mmu *mmu)
{
    return mmu->v_cpu;
}


/* Place a Write-Word request on the bus. */
int core_mmu_ww_send_cpu(struct core_mmu *mmu, uint16_t a, uint16_t v)
{
    mmu->pending_cpu = MMU_WRITE;
    mmu->a_cpu = a;
    mmu->v_cpu = v;
    mmu->vsz_cpu = 2;
    return 1;
}


/* VPU memory operations */
/* Place a Read-Byte request on the bus. */
int core_mmu_rb_send_vpu(struct core_mmu *mmu, uint16_t a)
{
    mmu->pending_vpu = MMU_READ;
    mmu->a_vpu = a;
    mmu->vsz_vpu = 1;
    return 1;
}


/* Return the result of the Read-Byte request from the MDR. */
uint8_t core_mmu_rb_fetch_vpu(struct core_mmu *mmu)
{
    return (uint8_t) mmu->v_vpu;
}


/* Place a Write-Byte request on the bus. */
int core_mmu_wb_send_vpu(struct core_mmu *mmu, uint16_t a, uint8_t v)
{
    mmu->pending_vpu = MMU_WRITE;
    mmu->a_vpu = a;
    mmu->v_vpu = v;
    mmu->vsz_vpu = 1;
    return 1;
}


/* Place a Read-Word request on the bus. */
int core_mmu_rw_send_vpu(struct core_mmu *mmu, uint16_t a)
{
    mmu->pending_vpu = MMU_READ;
    mmu->a_vpu = a;
    mmu->vsz_vpu = 2;
    return 1;
}


/* Return the result of the Read-Word request from the MDR. */
uint16_t core_mmu_rw_fetch_vpu(struct core_mmu *mmu)
{
    return mmu->v_vpu;
}


/* Place a Write-Word request on the bus. */
int core_mmu_ww_send_vpu(struct core_mmu *mmu, uint16_t a, uint16_t v)
{
    mmu->pending_vpu = MMU_WRITE;
    mmu->a_vpu = a;
    mmu->v_vpu = v;
    mmu->vsz_vpu = 2;
    return 1;
}


/* Apply any pending memory operations on the bus. */
void core_mmu_update(struct core_mmu *mmu)
{
    if(mmu->pending_cpu == MMU_READ) {
        if(mmu->vsz_cpu == 1)
            mmu->v_cpu = core_mmu_readb(mmu, mmu->a_cpu);
        else
            mmu->v_cpu = core_mmu_readw(mmu, mmu->a_cpu);
    } else if(mmu->pending_cpu == MMU_WRITE) {
        if(mmu->vsz_cpu == 1)
            core_mmu_writeb(mmu, mmu->a_cpu, mmu->v_cpu);
        else
            core_mmu_writew(mmu, mmu->a_cpu, mmu->v_cpu);
    }
    mmu->pending_cpu = MMU_NONE;
    
    if(mmu->pending_vpu == MMU_READ) {
        if(mmu->vsz_vpu == 1)
            mmu->v_vpu = core_mmu_readb(mmu, mmu->a_vpu);
        else
            mmu->v_vpu = core_mmu_readw(mmu, mmu->a_vpu);
    } else if(mmu->pending_vpu == MMU_WRITE) {
        if(mmu->vsz_vpu == 1)
            core_mmu_writeb(mmu, mmu->a_vpu, mmu->v_vpu);
        else
            core_mmu_writew(mmu, mmu->a_vpu, mmu->v_vpu);
    }
    mmu->pending_vpu = MMU_NONE;
}


/*---------------------------------------------------------------------------*/

/* Check for a pending memory access. */
static inline int core_mmu_pending_cpu(struct core_mmu *mmu)
{
    return mmu->pending_cpu != MMU_NONE;
}

/* Check for a pending memory access. */
static inline int core_mmu_pending_vpu(struct core_mmu *mmu)
{
    return mmu->pending_vpu != MMU_NONE;
}

/* Read a byte from the correct device/bank for that address. */
static uint8_t core_mmu_readb(struct core_mmu *mmu, uint16_t a)
{
    /* Check which memory bank to access, or which handler to use. */
    if(a <= A_ROM_FIXED_END)
        return mmu->rom_f[a - A_ROM_FIXED];
    else if(a <= A_ROM_SWAP_END)
        return mmu->rom_s[a - A_ROM_SWAP];
    else if(a <= A_RAM_FIXED_END)
        return mmu->ram_f[a - A_RAM_FIXED];
    else if(a <= A_RAM_SWAP_END)
        return mmu->ram_s[a - A_RAM_SWAP];
    else if(a <= A_TILE_SWAP_END) {
        return mmu->tile_s[a - A_TILE_SWAP];
    } else if(a <= A_VPU_END)
        return core_vpu_readb(mmu->vpu, a);
    else if(a <= A_APU_END)
        return 0;//mmu->apu_readb(a);
    else if(a <= A_DPCM_SWAP_END)
        return mmu->dpcm_s[a - A_DPCM_SWAP];
    else if(a <= A_CART_FIXED_END)
        return mmu->cart_f[a - A_CART_FIXED];
    else if(a == A_ROM_BANK_SELECT)
        return mmu->rom_s_bank;
    else if(a == A_RAM_BANK_SELECT)
        return mmu->ram_s_bank;
    else if(a == A_HIRES_CTR)
        return core_cpu_hrc_getlob(mmu->cpu->hrc);
    else if(a == A_HIRES_CTR + 1)
        return core_cpu_hrc_gethib(mmu->cpu->hrc);
    else if(a <= A_SERIAL_REG_END)
        LOGV("core.mmu: write @ address $%04x: serial stub", a);
    else if(a >= A_PAD1_REG && a <= A_PAD1_REG_END)
        LOGV("core.mmu: read  @ address $%04x: gamepad stub", a);
    else if(a >= A_PAD2_REG && a <= A_PAD2_REG_END)
        LOGV("core.mmu: read  @ address $%04x: gamepad stub", a);
    else if(a <= A_INT_VEC_END) {
        LOGV("core.mmu: read @ address $%04x: $%02x (p: $%04x)", a,
             mmu->intvec[a - A_INT_VEC], mmu->cpu->r[R_P]);
        return mmu->intvec[a - A_INT_VEC];
    }
    else {
        LOGW("core.mmu: read  @ address $%04x: unhandled", a);
        return 0;
    }
}


/* Write a byte to the correct device/bank part for that address. */
static void core_mmu_writeb(struct core_mmu *mmu, uint16_t a, uint8_t v)
{
    /* Check which memory bank to access, or which handler to use. */
    if(a <= A_ROM_FIXED_END)
        mmu->rom_f[a - A_ROM_FIXED] = v;
    else if(a <= A_ROM_SWAP_END)
        mmu->rom_s[a - A_ROM_SWAP] = v;
    else if(a <= A_RAM_FIXED_END)
        mmu->ram_f[a - A_RAM_FIXED] = v;
    else if(a <= A_RAM_SWAP_END)
        mmu->ram_s[a - A_RAM_SWAP] = v;
    else if(a <= A_TILE_SWAP_END) {
        mmu->tile_s[a - A_TILE_SWAP] = v;
    } else if(a == A_TILE_BANK_SELECT)
        core_mmu_bank_select(mmu, B_TILE_SWAP, v);
    else if(a <= A_VPU_END)
        core_vpu_writeb(mmu->vpu, a, v);
    else if(a == A_DPCM_BANK_SELECT)
        core_mmu_bank_select(mmu, B_DPCM_SWAP, v);
    else if(a <= A_APU_END)
        ;//mmu->apu_writeb(a, v);
    else if(a <= A_DPCM_SWAP_END)
        mmu->dpcm_s[a - A_DPCM_SWAP] = v;
    else if(a <= A_FIXED0_END)
        LOGW("core.mmu: write @ address $%04x: unhandled", a);
    else if(a <= A_CART_FIXED_END)
        mmu->cart_f[a - A_CART_FIXED] = v;
    else if(a <= A_FIXED1_END)
        LOGW("core.mmu: write @ address $%04x: unhandled (p:$%04x)", a, mmu->cpu->r[R_P]);
    else if(a == A_ROM_BANK_SELECT)
        core_mmu_bank_select(mmu, B_ROM_SWAP, v);
    else if(a == A_RAM_BANK_SELECT)
        core_mmu_bank_select(mmu, B_RAM_SWAP, v);
    else if(a == A_HIRES_CTR)
        core_cpu_hrc_setlob(mmu->cpu->hrc, v);
    else if(a == A_HIRES_CTR + 1)
        core_cpu_hrc_sethib(mmu->cpu->hrc, v);
    else if(a <= A_SERIAL_REG_END)
        LOGV("core.mmu: write @ address $%04x: serial stub", a);
    else if(a <= A_INT_VEC_END) {
        LOGV("core.mmu: write @ address $%04x: $%02x (p:$%04x)", a, v,
             mmu->cpu->r[R_P]);
        mmu->intvec[a - A_INT_VEC] = v;
    }
    else {
        LOGW("core.mmu: write @ address $%04x: unhandled (p:$%04x)", a, mmu->cpu->r[R_P]);
    }
}


/* Read a word from the correct device/bank part for that address. */
static uint16_t core_mmu_readw(struct core_mmu *mmu, uint16_t a)
{
    uint16_t result = 0;

    result |= core_mmu_readb(mmu, a);
    result |= core_mmu_readb(mmu, a + 1) << 8;

    return result;
}


/* Write a word to the correct device/bank part for that address. */
static void core_mmu_writew(struct core_mmu *mmu, uint16_t a, uint16_t v)
{
    core_mmu_writeb(mmu, a, (v & 0xff));
    core_mmu_writeb(mmu, a + 1, v >> 8);
}

