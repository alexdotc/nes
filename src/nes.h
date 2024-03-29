#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "mem.h"
#include "ppu.h"

typedef struct NES{
    CPU cpu;
    PPU ppu;
    Memory mem; /* CPU memory map */
    PPUMemory ppumem; /* PPU memory map */
    /* ... */
} NES;

NES power_on();
void power_off(NES*);

#endif
