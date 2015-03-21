/*
 * core/cpu/hrc.h -- CPU high resolution counter (header).
 *
 *
 */

#ifndef QPRA_CORE_HRC_H
#define QPRA_CORE_HRC_H

#define _POSIX_C_SOURCE 199309L
#include <time.h>

#include "core/cpu/cpu.h"

static const int CPU_FREQ_HZ = 3932160;

enum core_hrc_type {
    HRC_DISABLED = 0,
    HRC_60HZ, HRC_120HZ, HRC_240HZ, HRC_480HZ, HRC_960HZ,
    HRC_DISABLED6, HRC_DISABLED7
};

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

struct core_hrc {
    uint8_t v;

    enum core_hrc_type type;
    int elapsed_hz;
    int elapsed_us;

    struct timespec start;
    struct timespec cur;
};

static inline void core_cpu_hrc__diff(struct core_hrc *hrc)
{
    int old_hz = hrc->elapsed_hz;
    hrc->elapsed_us = (hrc->cur.tv_sec - hrc->start.tv_sec) * 1000 +
        (hrc->cur.tv_nsec - hrc->start.tv_nsec) / 1000;
    hrc->elapsed_hz = ((long)CPU_FREQ_HZ * (long)hrc->elapsed_us) / 1000;
    hrc->v -= hrc->elapsed_hz - old_hz;
}

static inline void core_cpu_hrc__trigger_int(struct core_cpu *cpu)
{
    cpu->interrupt = INT_TIMER_IRQ;
}

void core_cpu_hrc_init(struct core_cpu *);

void core_cpu_hrc_step(struct core_cpu *);
void core_cpu_hrc_write(struct core_cpu *, uint8_t);
uint8_t core_cpu_hrc_read(struct core_cpu *);

#endif

