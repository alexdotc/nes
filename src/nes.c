#include "nes.h"
#include "cpu.h"
#include "mem.h"
#include "rom.h"

NES power_on(){
    Memory mem = alloc_main_memory();
    CPU cpu = make_cpu(&mem);
    NES nes = { cpu, mem };
    load_rom(&nes, "ex.nes");
    reset(&cpu);
    return nes;
}

void power_off(NES* nes){
    /* TODO */

    free_main_memory(nes->mem);

}
