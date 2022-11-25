#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "mem.h"

typedef struct NES{
    CPU cpu;
    Memory mem;
    /* ... */
} NES;

NES power_on();
void power_off(NES*);

#endif
