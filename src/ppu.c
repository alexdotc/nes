#include "mem.h"
#include "ppu.h"
#include "util.h"

PPU make_ppu(PPUMemory* mem){
    PPU ppu = {0, 0, 0xA0, 0, 0, 0, 0, 0, mem};
    return ppu;
}
