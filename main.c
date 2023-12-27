#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define ARENA_IMPLEMENTATION
#include "./arena.h"

#define FILEPATH "./roms/super-mario-bros.nes"

#define array_len(arr) sizeof(arr) / sizeof((arr)[0])

typedef unsigned char byte;

typedef struct {
    byte *prg;
    byte *chr;
    byte *sram;
    byte mapper;
    byte mirror;
    bool battery;
} Cartridge;

typedef struct {
} NES;

void read_bytes(void *buffer, size_t count, FILE *stream) {
    size_t read = fread(buffer, sizeof(char), count, stream);
    if (read != count) {
        fprintf(stderr, "Error: cound not read %zu bytes from buffer\n", count);
        exit(1);
    }
}

byte read_byte(FILE *stream) {
    byte byte;
    read_bytes(&byte, 1, stream);
    return byte;
}

Cartridge cartridge_from_ines_file(const char *filepath, Arena *arena) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open `%s`: %s\n", filepath, strerror(errno));
        exit(1);
    }

    byte expected_magic[] = { 'N', 'E', 'S', 0x1A };
    const size_t magic_size = array_len(expected_magic);
    byte magic[magic_size];
    read_bytes(magic, magic_size, file);

    if (memcmp(magic, expected_magic, magic_size) != 0) {
        fprintf(stderr, "Error: file is not valid nes format!\n");
        exit(1);
    }

    byte prg_rom_size = read_byte(file);
    byte chr_rom_size = read_byte(file);

    byte flag_06 = read_byte(file);
    byte mirroring_arrangement = flag_06 & 0x01;
    bool contains_battery_backed_ram = (flag_06 & 0x02) != 0;
    bool contains_trainer = (flag_06 & 0x04) != 0;
    byte four_screen_vram = (flag_06 & 0x08);
    byte lower_nybble_mapper = flag_06 & 0xF0;

    // NOTE(nic): we are ignoring the first 2 bits of the flag
    // Because I have no idea about what VS Unisystem or PlayChoice-10 are
    byte flag_07 = read_byte(file);
    bool is_nes2 = (((flag_07 & 0x0C) << 4) == 2);
    byte upper_nybble_mapper = flag_07 & 0xF0;

    if (is_nes2) {
        fprintf(stderr, "Error: we do not support the NES 2.0 format!\n");
        exit(1);
    }

    // NOTE(nic): PRG RAM size, ignoring it for now
    // even if zero, still means we have 8KB reserved for it
    (void) read_byte(file);

    // NOTE(nic): I really don't want to deal with flags 9 and 10
    (void) read_byte(file);
    (void) read_byte(file);

    const size_t pad_size = 5;
    byte expected_padding[pad_size];
    memset(expected_padding, 0, pad_size);

    byte padding[pad_size];
    read_bytes(padding, pad_size, file);

    if (memcmp(padding, expected_padding, pad_size) != 0) {
        fprintf(stderr, "Error: zero padding is not respected in file!\n");
        exit(1);
    }

    // Reading trainer if present, ignoring it for now
    if (contains_trainer) {
        const size_t trainer_size = 512;
        byte trainer[trainer_size];
        read_bytes(&trainer, trainer_size, file);
    }

    size_t prg_size = prg_rom_size * 16384;
    size_t chr_size = chr_rom_size * 8192;

    byte *prg_rom = arena_alloc(arena, prg_size);
    byte *chr_rom = arena_alloc(arena, chr_size);
    byte *sram = arena_alloc(arena, 8192);

    read_bytes(prg_rom, prg_size, file);
    read_bytes(chr_rom, chr_size, file);

    fclose(file);

    byte mapper = ((lower_nybble_mapper >> 4) | upper_nybble_mapper);
    byte mirroring = mirroring_arrangement | four_screen_vram;

    Cartridge c = { 0 };
    c.prg = prg_rom;
    c.chr = chr_rom;
    c.sram = sram;
    c.mapper = mapper;
    c.mirror = mirroring;
    c.battery = contains_battery_backed_ram;
    return c;
}

int main(void) {
    Arena arena = { 0 };
    Cartridge cartridge = cartridge_from_ines_file(FILEPATH, &arena);

    // What now?

    arena_free(&arena);
    return 0;
}
