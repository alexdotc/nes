#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "mem.h"

#define NMI 0xFFFA
#define RESET 0xFFFC
#define IRQ 0xFFFE 

#define IRQ_DISABLE 0x34
#define SP_INIT 0xFD

typedef struct CPU {

    /* registers */
    uint8_t A, X, Y, S, P;
    uint16_t PC;

    /* memory */
    Memory* mem;
    
} CPU;

CPU make_cpu(Memory*);
void reset(CPU*);

#endif
