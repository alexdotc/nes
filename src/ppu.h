#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#include "mem.h"

typedef struct PPU{

    uint8_t ppuctrl;
    uint8_t ppumask;
    uint8_t ppustatus;
    uint8_t oamaddr;
    uint8_t oamdata;
    uint8_t ppuscroll;
    uint8_t ppuaddr;
    uint8_t ppudata;

    PPUMemory* ppumemory;

} PPU;

PPU make_ppu(PPUMemory*);

#endif
