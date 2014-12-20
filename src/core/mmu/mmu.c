/*
 * core/mmu/mmu.c -- Emulator MMU functions.
 *
 * The functions used by the Memory Management Unit.
 *
 */

#include <stdlib.h>
#include "core/mmu/mmu.h"
#include "log.h"

int core_mmu_init(struct core_mmu **pmmu, struct core_mmu_params *params)
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
    rom_f = calloc(16*1024, sizeof(uint8_t));
    if(rom_f == NULL)
        goto l_malloc_error;
    mmu->rom_f = rom_f;

    ram_f = calloc(8*1024, sizeof(uint8_t));
    if(ram_f == NULL)
        goto l_malloc_error;
    mmu->ram_f = ram_f;

    /* Allocate the cart permanent storage. */
    cart_f = calloc(256, sizeof(uint8_t));
    if(cart_f == NULL)
        goto l_malloc_error;
    mmu->cart_f = cart_f;

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
        rom_s[i] = calloc(16*1024, sizeof(uint8_t));
        if(rom_s[i] == NULL)
            goto l_malloc_error;
    }

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
        ram_s[i] = calloc(8*1024, sizeof(uint8_t));
        if(ram_s[i] == NULL)
            goto l_malloc_error;
    }

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
        tile_s[i] = calloc(8*1024, sizeof(uint8_t));
        if(tile_s[i] == NULL)
            goto l_malloc_error;
    }

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
        dpcm_s[i] = calloc(2*1024, sizeof(uint8_t));
        if(dpcm_s[i] == NULL)
            goto l_malloc_error;
    }
   
    /* Everything was allocated properly, phew. */
    LOGD("Allocated: %hhu ROM bank%s, %hhu RAM bank%s, %hhu tile ROM bank%s,"
         " %hhu DPCM ROM bank%s",
         params->rom_banks, params->rom_banks > 1 ? "s" : "",
         params->ram_banks, params->ram_banks > 1 ? "s" : "",
         params->tile_banks, params->tile_banks > 1 ? "s" : "",
         params->dpcm_banks, params->dpcm_banks > 1 ? "s" : "");

    /* Set up the callbacks for memory access in other parts of the system. */
    if(params->vpu_readb == NULL) {
        LOGE("Invalid VPU byte read callback");
        return 0;
    }
    mmu->vpu_readb = params->vpu_readb;
    if(params->vpu_writeb == NULL) {
        LOGE("Invalid VPU byte write callback");
        return 0;
    }
    mmu->vpu_writeb = params->vpu_writeb;
    if(params->apu_readb == NULL) {
        LOGE("Invalid APU byte read callback");
        return 0;
    }
    mmu->apu_readb = params->apu_readb;
    if(params->apu_writeb == NULL) {
        LOGE("Invalid APU byte write callback");
        return 0;
    }
    mmu->apu_writeb = params->apu_writeb;

    return 1;

l_malloc_error:
    LOGE("Failed to allocate memory for banks");
    return 0;
}

int core_mmu_destroy(struct core_mmu *mmu)
{
    int i;

    free(rom_f);
    mmu->rom_f = rom_f = NULL;
    free(ram_f);
    mmu->ram_f = ram_f = NULL;
    free(cart_f);
    mmu->cart_f = cart_f = NULL;

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

    return 1;
}

int core_mmu_bank_select(struct core_mmu *mmu, enum core_mmu_bank bank,
                         uint8_t index)
{
    switch(bank) {
        case B_ROM_FIXED:
        case B_RAM_FIXED:
            LOGE("Attempted to switch fixed memory bank");
            return 0;
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

uint8_t core_mmu_readb(struct core_mmu *mmu, uint16_t a)
{
    /* Check which memory bank to access, or which handler to use. */
    if(a < A_ROM_SWAP)
        return mmu->rom_f[a];
    else if(a < A_RAM_FIXED)
        return mmu->rom_s[a];
    else if(a < A_RAM_SWAP)
        return mmu->ram_f[a];
    else if(a < A_TILE_SWAP)
        return mmu->ram_s[a];
    else if(a < A_TILE_SWAP_END)
        return mmu->tile_s[a];
    else if(a < A_VPU_END)
        return mmu->vpu_readb(a);
    else if(a < A_APU_END)
        return mmu->apu_readb(a);
    else if(a < A_DPCM_SWAP_END)
        return mmu->dpcm_s[a];
    else if(a < A_CART_FIXED_END)
        return mmu->cart_f[a];
    else if(a == A_ROM_BANK_SELECT)
        return mmu->rom_s_bank;
    else if(a == A_RAM_BANK_SELECT)
        return mmu->ram_s_bank;
    else {
        LOGW("unhandled address $%04x read", a);
        return 0;
    }
}

void core_mmu_writeb(struct core_mmu *mmu, uint16_t a, uint8_t v)
{
    /* Check which memory bank to access, or which handler to use. */
    if(a < A_ROM_SWAP)
        mmu->rom_f[a] = v;
    else if(a < A_RAM_FIXED)
        mmu->rom_s[a] = v;
    else if(a < A_RAM_SWAP)
        mmu->ram_f[a] = v;
    else if(a < A_TILE_SWAP)
        mmu->ram_s[a] = v;
    else if(a < A_TILE_SWAP_END)
        mmu->tile_s[a] = v;
    else if(a < A_VPU_END)
        mmu->vpu_writeb(a, v);
    else if(a < A_APU_END)
        mmu->apu_writeb(a, v);
    else if(a < A_DPCM_SWAP_END)
        mmu->dpcm_s[a] = v;
    else if(a < A_CART_FIXED_END)
        mmu->cart_f[a] = v;
    else if(a < A_ROM_BANK_SELECT)
        mmu->rom_s_bank = v;
    else if(a < A_RAM_BANK_SELECT)
        mmu->ram_s_bank = v;
    else {
        LOGW("unhandled address $%04x write", a);
    }
}

