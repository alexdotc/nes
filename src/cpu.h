#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "mem.h"

#define NMI 0xFFFA
#define RESET 0xFFFC
#define IRQ 0xFFFE 

#define STATUS_INIT 0x34 
/* Regarding this from nesdev wiki: "The golden log of nestest differs from this in the irrelevant bits 5 and 4 of P"...
   To elaborate: My understanding is that we need IRQ_DISABLE bit 2 set and bits 7,6,3,1,0 not set.
   0x24 (0010 0100) or 0x04 (0000 0100) are also valid initial power-on states for the status register */


#define SP_INIT 0xFD

typedef struct CPU {

    /* registers */
    uint8_t A, X, Y, P, SP;
    uint16_t PC;

    /* memory */
    Memory* mem;
    
} CPU;

CPU make_cpu(Memory*);
void reset(CPU*);
void FDE(CPU*);

#endif
