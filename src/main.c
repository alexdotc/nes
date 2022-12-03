#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "nes.h"
#include "util.h"

typedef struct Options{
    const char *rom_filename;
} Options;

static Options* parse_options(int argc, char *const argv[]);

int main(int argc, char *const argv[]){

    Options* options = parse_options(argc, argv);
    if (options->rom_filename == NULL) 
        err_exit("No ROM provided");

    printf("Power on\n");
    NES nes = power_on(options->rom_filename);

    printf("Power off\n");
    power_off(&nes);

    free(options);

    return 0;

}

static Options* parse_options(int argc, char *const argv[]){
    Options *options = xalloc(1, sizeof(Options), twoarg_malloc);
    if (options == NULL) return options;
    options->rom_filename = NULL;

    /*  int opt;
        while((opt = getopt(argc, argv, "x:s:")) != -1)
        switch(opt){
            case 'x': options->scale = strtol(optarg, NULL, 0); break;
            case 's': options->speed = strtol(optarg, NULL, 0); break;
            default: ;
        }
    */

    if (optind < argc) options->rom_filename = argv[optind++];

    return options;
}
