#include "nes.h"
#include "cpu.h"
#include "mem.h"

NES power_on(){
    Memory mem = alloc_main_memory();
    /* Cart cart = load_rom();
     * LOAD INES ROM AND MAP TO MEMORY */
    CPU cpu = make_cpu(&mem);
    reset(&cpu);

    NES nes = { cpu, mem };

    return nes;
}

void power_off(NES* nes){
    /* TODO */

    free_main_memory(nes->mem);

}
