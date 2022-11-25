#include <stdint.h>

#include "cpu.h"
#include "mem.h"

CPU make_cpu(Memory* mem){
    uint8_t A = 0, X = 0, Y = 0;
    uint8_t P = IRQ_DISABLE;
    uint8_t S = SP_INIT;
    uint16_t PC = 0;

    CPU cpu = { A,X,Y,S,P,PC,mem };

    return cpu;
}

void reset(CPU* cpu){
    /* set PC to address in reset vector */

    cpu->PC = (((uint16_t)(*(cpu->mem->map[RESET+1]))) << 8) + (uint16_t)(*(cpu->mem->map[RESET])); /* TODO make a 2-byte LE parser function */

}
