#ifndef ROM_H
#define ROM_H

#include <stdint.h>
#include <stdbool.h>

#include "nes.h"

#define INES_SIGNATURE 0x1A53454E /* little-endian hack to read as uint32 (reversed) */
#define PRGROM_PAGESIZE 16384
#define CHRROM_PAGESIZE 8192
#define PRGROM_START 0x8000 /* start mapping rom at this memory location */

typedef struct InesHeader {
    /* header for ines rom */
    bool valid_signature; /* matches 0x4E45531A "NES\SUB" */
    uint8_t prgrom; /* number of 16K PRG ROM pages */
    uint8_t chrrom; /* number of 8K CHR ROM page */
    uint8_t mapper;
    /* TODO add more stuff when needed */

} InesHeader;

const InesHeader load_rom(NES*, const char*);

#endif
