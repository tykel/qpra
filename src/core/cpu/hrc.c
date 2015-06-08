/*
 * core/cpu/hrc.c -- CPU high resolution counter.
 *
 *
 */

#include <string.h>

#include "core/cpu/hrc.h"
#include "log.h"


/* Signal an interrupt request (IRQ) for the next instruction. */
static inline void core_cpu_hrc__trigger_int(struct core_cpu *cpu)
{
    cpu->interrupt = INT_TIMER_IRQ;
}


/* Initialize the HRC state: simply zero all variables. */
void core_cpu_hrc_init(struct core_cpu *cpu)
{
    memset(cpu->hrc, 0, sizeof(struct core_hrc));
}


/* Step forward one time unit, and update. Will fire an interrupt if ready. */
void core_cpu_hrc_step(struct core_cpu *cpu)
{
    struct core_hrc *hrc = cpu->hrc;

    if(hrc->v & 1 && !hrc->enabled) {
        if(hrc->v & 2) {
            /* Determine number of cycles until next horizontal sync pulse. */
            hrc->total_cycles = VPU_SCANLINE_CYCLES -
                    (cpu->total_cycles % VPU_SCANLINE_CYCLES);
        } else {
            /* Plain old cycle timing. */
            hrc->total_cycles = (hrc->v & 0xfffc) << 2;
        }
        hrc->enabled = 1;
        LOGV("core.cpu: timer enabled, delay = %d", hrc->total_cycles);
    } else if(!(hrc->v & 1) && hrc->enabled) {
        hrc->enabled = 0;
        LOGV("core.cpu: timer disabled");
    }

    if(!hrc->enabled)
        return;

    /* Update the elapsed time; if we have reached one counter cycle, trigger
     * an interrupt. */
    hrc->elapsed_cycles += 1;
    if(hrc->total_cycles == hrc->elapsed_cycles) {
        core_cpu_hrc__trigger_int(cpu);
        /* Reset the timer for the next period. */
        hrc->elapsed_cycles = 0;
        if(hrc->v & 2) {
            /* We are at the start of H-SYNC, so the next one is exactly one
             * scanline away. */
            hrc->total_cycles = VPU_SCANLINE_CYCLES;
        }
    }
}


void core_cpu_hrc_sethib(struct core_hrc *hrc, int v)
{
    hrc->v |= v << 8;
}

void core_cpu_hrc_setlob(struct core_hrc *hrc, int v)
{
    hrc->v |= v & 0xff;
}


int core_cpu_hrc_gethib(struct core_hrc *hrc)
{
    return hrc->v >> 8;
}

int core_cpu_hrc_getlob(struct core_hrc *hrc)
{
    return hrc->v & 0xff;
}

