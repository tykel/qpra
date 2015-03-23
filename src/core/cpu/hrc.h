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

struct core_hrc {
    uint8_t v;

    enum core_hrc_type type;
    int elapsed_hz;
    int elapsed_us;

    struct timespec start;
    struct timespec cur;
};

void core_cpu_hrc_init(struct core_cpu *);

void core_cpu_hrc_step(struct core_cpu *);
void core_cpu_hrc_settype(struct core_hrc *, int);

#endif

