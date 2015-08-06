/*
 * Khepra global system v2 (Experimental)
 */

#include "system.h"

bool system_init(struct system_state *s)
{
    cpu_init(&s->cpu);
    vpu_init(&s->vpu, &s->cpu);
}

bool system_cycle(struct system_state *s)
{
    cpu_cycle(&s->cpu);
    vpu_cycle(&s->vpu);
}

bool system_destroy(struct system_state *s);
{
    cpu_destroy(&s->cpu);
    vpu_destroy(&s->cpu);
}
