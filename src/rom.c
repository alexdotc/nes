#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rom.h"
#include "util.h"

#define HEADER_LEN 16

const InesHeader read_ines_header(const uint8_t*);
void validate_ines_header(const InesHeader*, const char*);
void map_rom(NES*, const InesHeader*, uint8_t*);

const InesHeader read_ines_header(const uint8_t* header){
    /* Parse an ines header and to a struct 
       Does NOT validate anything */
    bool sig = (*((uint32_t*) header) == INES_SIGNATURE);
    /* TODO offset 4 is the LSB of a 12-bit value PRGROM size but
     * this is good enough for testing and any retail NES game. */
    uint8_t prgrom = header[4];
    uint8_t mapper = (header[7] & 0xF0) & (header[6] >> 4);
    const InesHeader h = { sig, prgrom, mapper };
    /* TODO update this as we need to read and support more stuff */

    return h;
}

void validate_ines_header(const InesHeader* header, const char* filename){
    if(header->valid_signature != true)
        err_exit("ROM: iNES signature mismatch while loading %s", filename);
    if(header->mapper != 0)
        err_exit("ROM: Mapper %d not supported while loading %s", header->mapper, filename);
    /* TODO update validation based on what we do and don't support. Maybe just return error to caller */
}

const InesHeader load_rom(NES* nes, const char* filename){

    FILE* rom = fopen(filename, "rb");
    if (rom == NULL)
        err_exit("ROM: Error opening file %s: %s", filename, strerror(errno));

    size_t read;

    /* read+validate header */
    uint8_t header_bytes[HEADER_LEN];
    read = fread(header_bytes, 1, HEADER_LEN, rom);
    if (read != HEADER_LEN) 
        err_exit("ROM: Couldn't read header from %s", filename);
    const InesHeader header = read_ines_header(header_bytes);
    validate_ines_header(&header, filename);

    /* read prgrom */
    uint8_t data[header.prgrom*PRGROM_PAGESIZE]; /* TODO consider heap alloc if these get several times bigger */
    read = fread(data, 1, header.prgrom*PRGROM_PAGESIZE, rom); /* TODO wrap freads for single size read */
    if (read != header.prgrom*PRGROM_PAGESIZE) 
        err_exit("ROM: PRGROM read failed while loading %s. Read %d of %d bytes specified in header", 
                  read, header.prgrom*PRGROM_PAGESIZE);
    map_rom(nes, &header, data);
    fclose(rom);

    return header;
}

void map_NROM_256(NES* nes, const InesHeader* header, uint8_t* data){
    unsigned int s = PRGROM_PAGESIZE * header->prgrom;
    uint8_t* dest = (nes->mem).map[PRGROM_START];
    memcpy(dest, data, s);
}

void map_NROM_128(NES* nes, const InesHeader* header, uint8_t* data){
    unsigned int nb = PRGROM_PAGESIZE * header->prgrom;
    uint8_t* dest = (nes->mem).map[PRGROM_START];
    memcpy(dest, data, nb);
    uint8_t** mirror = (nes->mem.map)+0xC000;
    uint8_t** src = (nes->mem.map)+PRGROM_START;
    memcpy(mirror, src, sizeof(uint8_t*) * nb); /* mirror */
}

void map_rom(NES* nes, const InesHeader* header, uint8_t* data){
    if (header->prgrom == 2) 
        map_NROM_256(nes, header, data);
    else
        map_NROM_128(nes, header, data);
}
