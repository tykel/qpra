/*
 * core/cpu/hrc.c -- CPU high resolution counter.
 *
 *
 */

#include <string.h>

#include "core/cpu/hrc.h"

int hrc_use_rtc = 1; 

int hrc_hz[] = {
    -1,
    60, 120, 240, 480, 960,
    -1, -1
};

int hrc_cycles[] = {
    -1,
    65536, 32768, 16384, 8192, 4096,
    -1, -1
};

int hrc_us[] = {
    -1,
    16667, 8334, 4167, 2084, 1042,
    -1, -1
};


/* Update the elapsed and remaining time value for the HRC. */
static inline void core_cpu_hrc__diff(struct core_hrc *hrc)
{
    int old_us = hrc->elapsed_us;
    if(hrc_use_rtc) {
        hrc->elapsed_us = (hrc->cur.tv_sec - hrc->start.tv_sec) * 1000 +
            (hrc->cur.tv_nsec - hrc->start.tv_nsec) / 1000;
        //hrc->elapsed_hz = ((long)CPU_FREQ_HZ * (long)hrc->elapsed_us) / 1000;
        hrc->v -= hrc->elapsed_us - old_us;
    } else {
        hrc->v -= 1;
    }
}


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

    if(hrc->type == HRC_DISABLED ||
       hrc->type == HRC_DISABLED6 || hrc->type == HRC_DISABLED7)
        return;

    /* Update the elapsed time; if we have reached one counter cycle, trigger
     * an interrupt. */
    clock_gettime(CLOCK_MONOTONIC, &hrc->cur);
    core_cpu_hrc__diff(hrc);
    if(hrc->v <= 0) {
        core_cpu_hrc__trigger_int(cpu);
        /* Reset the timer for the next period. */
        clock_gettime(CLOCK_MONOTONIC, &hrc->start);
        //hrc->elapsed_hz = 0;
        hrc->elapsed_us = 0;
        hrc->v = hrc_use_rtc ? hrc_us[hrc->type] : hrc_cycles[hrc->type];
    }
}


/* 
 * Set the timer mode, enabling it if the mode is not DISABLED.
 * Resets the HRC state.
 */
void core_cpu_hrc_settype(struct core_hrc *hrc, int type)
{
    switch(type) {
        case HRC_60HZ:
        case HRC_120HZ:
        case HRC_240HZ:
        case HRC_480HZ:
        case HRC_960HZ:
            hrc->type = type;
            break;
        case HRC_DISABLED:
        case HRC_DISABLED6:
        case HRC_DISABLED7:
        default:
            hrc->type = HRC_DISABLED;
            break;
    }

    clock_gettime(CLOCK_MONOTONIC, &hrc->start);
    hrc->elapsed_hz = 0;
    hrc->elapsed_us = 0;
    hrc->v = hrc_use_rtc ? hrc_us[hrc->type] : hrc_cycles[hrc->type];
}


/* Return the currently used counter mode. */
int core_cpu_hrc_gettype(struct core_hrc *hrc)
{
    return hrc->type;
}

