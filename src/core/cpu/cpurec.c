typedef void (*op)(void *);

struct cpurec_state {
    uint16_t p;

    // Map from PC to compiled buffer
    void* cache[64*1024];

    struct cpu_state *cpu;
};

void* cpurec_getbuf(struct cpurec_state *rec, uint16_t p)
{
    return rec->cache[p];
}

void cpurec_invalidate(struct cpurec_state *rec, uint16_t p)
{
    free(rec->cache[p]);
    rec->cache[p] = NULL;
}

static inline void advance(struct cpurec_state *rec)
{
    int m = mode(rec->cpu);
    rec->p += 2 + ((m == MODE_RIR || m == MODE_IR || m == MODE_PR || m == MODE_RP) && cpu->ib1 & 1) +
              ((m == MODE_P) && (cpu->ib0 & 1));
}

bool cpurec_compile(struct cpurec_state *rec)
{
    uint16_t start = rec->p;
    uint16_t end = start;
    int op;

    do {
        op = op(rec->cpu);
        advance(rec);
    } while(op != 14 && rec->p <= 0xfffe); // JP
    end = rec->p;
}