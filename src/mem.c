#include <stdint.h>
#include <stdlib.h>

#include "mem.h"
#include "util.h"

void init_main_memory(uint8_t*, uint8_t**, PPU* ppu);
void init_ppu_memory(uint8_t*, uint8_t**);

Memory alloc_main_memory(PPU* ppu){
    
    uint8_t* _map = xalloc(CPU_MEM_SIZE, sizeof(uint8_t), calloc);
    uint8_t** map = xalloc(CPU_MEM_SIZE, sizeof(uint8_t*), calloc);

    init_main_memory(_map, map, ppu);
    
    uint8_t** ram = map;
    uint8_t** ppu_reg = map+(0x2000);
    uint8_t** data_reg = map+(0x4000);
    uint8_t** test_reg = map+(0x4018);
    uint8_t** prg = map+(0x4020);

    Memory mem = { map, ram, ppu_reg, data_reg, test_reg, prg };

    return mem;
}

PPUMemory alloc_ppu_memory(void){
    uint8_t* _map = xalloc(PPU_MEM_SIZE, sizeof(uint8_t), calloc);
    uint8_t** map = xalloc(PPU_MEM_SIZE, sizeof(uint8_t*), calloc);

    init_ppu_memory(_map, map);

    uint8_t** pattern = map;
    uint8_t** nametable = map+(0x2000);
    uint8_t** no_render = map+(0x3000);
    uint8_t** palette = map+(0x3F00);

    PPUMemory mem = { map, pattern, nametable, no_render, palette };

    return mem;
}

void free_memory(FreeableMemory mem){
    
    free(*((mem.mem)->map)); /* free _map (nes real memory) */
    free((mem.mem)->map); /* free map (nes address space) */
}

void init_main_memory(uint8_t* _map, uint8_t** map, PPU* ppu){
    /* TODO use memcpy */

    /* 4x mirrored internal ram */
    for(int i = 0; i < 0x2000; ++i){
        map[i] = _map + (i % 0x800);
    }

    /* make the PPU registers into the address space */
    map[0x2000] = &(ppu->ppuctrl);
    map[0x2001] = &(ppu->ppumask);
    map[0x2002] = &(ppu->ppustatus);
    map[0x2003] = &(ppu->oamaddr);
    map[0x2004] = &(ppu->oamdata);
    map[0x2005] = &(ppu->ppuscroll);
    map[0x2006] = &(ppu->ppuaddr);
    map[0x2007] = &(ppu->ppudata);

    /* (1FFF / 8)x mirrored PPU registers */
    for(int i = 0x2008; i < 0x4000; ++i){
        map[i] = map[(i % 8) + 0x2000];
    }

    /* non-mirrored data registers, test registers, and cartridge space */
    for(int i = 0x4000; i < CPU_MEM_SIZE; ++i){
        map[i] = _map + i;
    }
}

void init_ppu_memory(uint8_t* _map, uint8_t** map){
    /* TODO use memcpy */

    /* non-mirrored pattern tables and nametables */
    for(int i = 0; i < 0x3000; ++i){
        map[i] = _map + i;
    }

    /* partial nametable mirror, usually unused and not rendered from */
    for(int i = 0x3000; i < 0x3EFF; ++i){
        map[i] = map[i-0x1000];
    }

    /* 2x mirrored palette control RAM */
    for(int i = 0x3F00; i < 0x3FFF; ++i){
        map[i] = _map + (i % 128);
    }
}
