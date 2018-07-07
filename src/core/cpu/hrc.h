/*
 * core/cpu/hrc.h -- CPU high resolution counter (header).
 *
 *
 */

#ifndef QPRA_CORE_HRC_H
#define QPRA_CORE_HRC_H

#include <time.h>

#include "core/cpu/cpu.h"

static const int CPU_FREQ_HZ = 5360520;
static const int VPU_SCANLINE_CYCLES_BEFORE_HSYNC = 316;
static const int VPU_SCANLINE_CYCLES = 341;

/* 
 * Hi-Res Counter structure.
 * Tracks timer mode, and start and elapsed time.
 */
struct core_hrc {
    /* The raw HRC register value. */
    uint16_t v;
    /* 
     * The counter will interrupt either after total_cycles, OR at each H-Sync;
     * not both.
     */
    int total_cycles;
    int hsync_override;
    /* Currently elapsed cycles. */
    int elapsed_cycles;
    /* Was the timer enabled at the previous cycle? */
    int enabled;
};

/* Function declarations. */
void core_cpu_hrc_init(struct core_cpu *);
void core_cpu_hrc_step(struct core_cpu *);
void core_cpu_hrc_setlob(struct core_hrc *, int);
void core_cpu_hrc_sethib(struct core_hrc *, int);
int core_cpu_hrc_getlob(struct core_hrc *);
int core_cpu_hrc_gethib(struct core_hrc *);

#endif

