#ifdef DEBUG
#include <stdio.h>
#endif

#include "nes.h"
#include "cpu.h"
#include "mem.h"
#include "rom.h"


NES power_on(const char* rom_filename){
    Memory mem = alloc_main_memory();
    CPU cpu = make_cpu(&mem);
    NES nes = { cpu, mem };
    load_rom(&nes, rom_filename); /* TODO at some point down the line, we probably just want to do this as something separate from power_on, and just wait for a call while idling */
    #ifdef DEBUG
    printf("Sampling NROM mirroring...\n");
    for (int i = 0x8000; i < 0x8010; ++i){
        printf("Location %04X: %02x\n", i, *(nes.mem.map[i]));
    }
    for (int i = 0xC000; i < 0xC010; ++i){
        printf("Location %04X: %02x\n", i, *(nes.mem.map[i]));
    }
    #endif
    reset(&cpu);
    for (int i = 0 ; i < 55; ++i)
        FDE(&cpu);
    return nes;
}

void power_off(NES* nes){
    /* TODO */

    free_main_memory(nes->mem);

}
