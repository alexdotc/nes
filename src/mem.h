#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stdbool.h>

#define CPU_MEM_SIZE 0xFFFF /* 64K memory map */
#define PPU_MEM_SIZE 0x3FFF /* 16K memory map */

typedef struct Memory{

    uint8_t** map;

    /* offset accessors */
    uint8_t** ram; /* $0000-$07FF, 2K internal RAM , mirrored 4 times to $1FFF */
    uint8_t** ppu_reg; /* $2000-$2007 */
    uint8_t** data_reg; /* $4000-$4017, apu and i/o registers */
    uint8_t** test_reg; /* $4018-$401F, disabled/cpu test registers */
    uint8_t** prg; /* $4020-$FFFF cartridge space (PRG ROM) */

} Memory;

typedef struct PPUMemory{

    uint8_t** map;

    /* offset accessors */
    uint8_t** pattern; /* $0000-$1FFF pattern tables (CHR ROM) */
    uint8_t** nametable; /* $2000-$2FFF nametables (RAM) */
    uint8_t** no_render; /* $3000-$3EFF partial mirror of the nametable space */
    uint8_t** palette; /* $3F00-$3FFF palette RAM and mirror */

} PPUMemory;

typedef union FreeableMemory {

    Memory* mem;
    PPUMemory* ppumem;

} FreeableMemory;

Memory alloc_main_memory(void);
void free_memory(FreeableMemory);

PPUMemory alloc_ppu_memory(void);

#endif
