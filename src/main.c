#include <stdio.h>

#include "nes.h"

int main(int argc, char* argv[]){

    printf("Power on\n");
    NES nes = power_on();

    printf("Power off\n");
    power_off(&nes);

    return 0;

}
