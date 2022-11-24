#include <stdio.h>

#include "cpu.h"
#include "mem.h"

int main(int argc, char* argv[]){

    printf("I've compiled without issue.\n");

    Memory mem = alloc_main_memory();

    printf("Mapped memory\n");
    printf("Verifying mirrors...\n");

    for(int i = 0; i < 0x2000; i = i + 0x100){
        if (!(i % 0x800)) printf("Sampling mirrors %d:\n", (i/0x800));
        printf("Mapped indirect %x to %p\n", i, mem.map[i]);
    }

    free_main_memory(mem);

    printf("Freed memory\n");

    return 0;

}
