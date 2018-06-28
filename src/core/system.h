#ifndef __system_h__
#define __system_h__

#include "core/common.h"
#include "core/cpu/cpu.h"
#include "core/vpu/vpu.h"

struct system_state {
    struct cpu_state cpu;
    struct vpu_state vpu;
};

bool system_init(struct system_state *);
bool system_cycle(struct system_state *);
bool system_destroy(struct system_state *);

#endif
