#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "mem.h"

typedef struct CPU {

    /* registers */
    uint8_t A, X, Y, S, P;
    uint16_t PC;

    /* memory */
    Memory* mem;
    
} CPU;

#endif
