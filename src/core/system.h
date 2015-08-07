#ifndef __system_h__
#define __system_h__

#include "common.h"
#include "cpu.h"
#include "vpu.h"

struct system_state {
    struct cpu_state cpu;
    struct vpu_state vpu;
};

bool system_init(struct system_state *);
bool system_cycle(struct system_state *);
bool system_destroy(struct system_state *);

#endif
