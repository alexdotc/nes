#include "nes.h"
#include "cpu.h"
#include "mem.h"
#include "rom.h"

#include <stdio.h>

NES power_on(){
    Memory mem = alloc_main_memory();
    CPU cpu = make_cpu(&mem);
    NES nes = { cpu, mem };
    load_rom(&nes, "nestest.nes");
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
    FDE(&cpu);
    FDE(&cpu);
    return nes;
}

void power_off(NES* nes){
    /* TODO */

    free_main_memory(nes->mem);

}
