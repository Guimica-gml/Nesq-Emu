#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

typedef unsigned char byte;

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

#define FILEPATH "./roms/super-mario-bros.nes"
#define MAGIC_SIZE 4

int main(void) {
    FILE *file = fopen(FILEPATH, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open `%s`: %s\n", FILEPATH, strerror(errno));
        exit(1);
    }

    byte expected_magic[MAGIC_SIZE] = { 'N', 'E', 'S', 0x1A };
    byte magic[MAGIC_SIZE];
    read_bytes(magic, MAGIC_SIZE, file);

    if (memcmp(magic, expected_magic, MAGIC_SIZE) != 0) {
        fprintf(stderr, "Error: file is not valid nes format!\n");
        exit(1);
    }

    byte prg_rom_size = read_byte(file);
    byte chr_rom_size = read_byte(file);

    // NOTE(nic): ignoring the third bit, because
    // I don't know what the trainer is yet
    byte flag_06 = read_byte(file);
    byte mirroring = flag_06 & 0x01;
    bool contains_prg_ram = (flag_06 & 0x02) != 0;
    bool four_screen_vram = (flag_06 & 0x08) != 0;
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

    // NOTE(nic): even if zero, still means we have 8KB reserved for it
    byte flag_08 = read_byte(file); // PRG RAM size

    // NOTE(nic): I really don't want to deal with flags 9 and 10
    (void) read_byte(file);
    (void) read_byte(file);

    const size_t pad_size = 5;
    byte expected_padding[pad_size];
    memset(expected_padding, 0, pad_size);

    byte padding[pad_size];
    read_bytes(padding, pad_size, file);

    if (memcmp(padding, expected_padding, pad_size) != 0) {
        fprintf(stderr, "Error: padding is not respected!\n");
        exit(1);
    }

    byte mapper = ((lower_nybble_mapper >> 4) | upper_nybble_mapper);

    printf("Prg_Rom_Size: %d (* 16KB = %dKB)\n", prg_rom_size, prg_rom_size * 16);
    printf("Chr_Rom_Size: %d (* 8KB = %dKB)\n", chr_rom_size, chr_rom_size * 8);
    printf("Prg_Ram_Size: %d (8KB even if zero)\n", flag_08);

    printf("Contains_Prg_Ram: %d\n", contains_prg_ram);
    printf("Four_Screen_Vram: %d\n", four_screen_vram);

    printf("Mirroring: %d\n", mirroring);
    printf("Mapper: %d\n", mapper);

    fclose(file);
    return 0;
}
