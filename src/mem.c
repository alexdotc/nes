#include <stdint.h>
#include <stdlib.h>

#include "mem.h"
#include "util.h"

void init_main_memory(uint8_t*, uint8_t**);

Memory alloc_main_memory(void){
    
    uint8_t* _map = (uint8_t  *)xmalloc(sizeof(uint8_t)  * CPU_MEM_SIZE);
    uint8_t** map = (uint8_t **)xmalloc(sizeof(uint8_t*) * CPU_MEM_SIZE);

    init_main_memory(_map, map);
    
    uint8_t** internal = map;
    uint8_t** ppu = map+(0x2000);
    uint8_t** data_reg = map+(0x4000);
    uint8_t** test_reg = map+(0x4018);
    uint8_t** cartridge = map+(0x4020);

    Memory mem = { map, internal, ppu, data_reg, test_reg, cartridge };

    return mem;
}

void free_main_memory(Memory mem){
    
    free(*(mem.map)); /* free _map */
    free(mem.map); /* free map */

}

void init_main_memory(uint8_t* _map, uint8_t** map){

    /* 4x mirrored internal ram */
    for(int i = 0; i < 0x2000; ++i){
        map[i] = _map + (i % 0x800);
    }

    /* (1FFF / 8)x mirrored PPU registers */
    for(int i = 0x2000; i < 0x4000; ++i){
        map[i] = _map + (i % 8);
    }

    /* non-mirrored data registers, test registers, and cartridge space */
    for(int i = 0x4000; i < CPU_MEM_SIZE; ++i){
        map[i] = _map + i;
    }
    
}
