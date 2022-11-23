#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stdbool.h>

#define CPU_MEM_SIZE 0xFFFF /* 64K memory map */

typedef struct Memory{

    uint8_t** map;

    /* convenient accessors */
    uint8_t** internal; /* $0000-$07FF, 2K internal RAM */
    uint8_t** ppu; /* $2000-$2007 */
    uint8_t** data_reg; /* $4000-$4017, apu and i/o registers */
    uint8_t** test_reg; /* $4018-$401F, disabled/cpu test registers */
    uint8_t** cartridge; /* $4020-$FFFF cartridge space */

} Memory;

Memory alloc_main_memory(void); /* the actual memory array is on the heap */
void free_main_memory(Memory);

#endif
