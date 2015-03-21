/*
 * core/cpu/hrc.c -- CPU high resolution counter.
 *
 *
 */

#include <string.h>

#include "core/cpu/hrc.h"

void core_cpu_hrc_init(struct core_cpu *cpu)
{
    memset(cpu->hrc, 0, sizeof(struct core_hrc));
}

void core_cpu_hrc_step(struct core_cpu *cpu)
{
    struct core_hrc *hrc = cpu->hrc;

    if(hrc->type == HRC_DISABLED ||
       hrc->type == HRC_DISABLED6 || hrc->type == HRC_DISABLED7)
        return;

    clock_gettime(CLOCK_MONOTONIC, &hrc->cur);
    core_cpu_hrc__diff(hrc);
    if(hrc->elapsed_hz >= hrc_hz[hrc->type])
        core_cpu_hrc__trigger_int(cpu);
}

void core_cpu_hrc_write(struct core_cpu *cpu, uint8_t v)
{
    cpu->hrc->v = v;
}

uint8_t core_cpu_hrc_read(struct core_cpu *cpu)
{
    return cpu->hrc->v;
}

