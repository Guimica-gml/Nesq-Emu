#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#define ARENA_IMPLEMENTATION
#include "./arena.h"

#define FILEPATH "./roms/super-mario-bros.nes"

#define array_len(arr) sizeof(arr) / sizeof((arr)[0])

typedef uint8_t byte;
typedef uint16_t u16;

// TODO: rewrite this to make sure it's on the same order
// as the Addresing_Mode enum
const size_t instruction_modes[256] = {
    6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
    1, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
    6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
    6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 8, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
    5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
    5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
    5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
    5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
    10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
};

const size_t instruction_sizes[256] = {
    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    1, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    1, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 0, 3, 0, 0,
    2, 2, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
};

const byte instruction_cycles[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

// NOTE(nic): indicates the number of cycles used by each
// instruction when a page is crossed, this table may not be necessary
const byte instruction_page_cycles[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
};

const char *instruction_names[256] = {
    "BRK", "ORA", "JAM", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
    "BPL", "ORA", "JAM", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
    "JSR", "AND", "JAM", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
    "BMI", "AND", "JAM", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
    "RTI", "EOR", "JAM", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE",
    "BVC", "EOR", "JAM", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
    "RTS", "ADC", "JAM", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
    "BVS", "ADC", "JAM", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
    "NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "XAA", "STY", "STA", "STX", "SAX",
    "BCC", "STA", "JAM", "AHX", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "AHX",
    "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX",
    "BCS", "LDA", "JAM", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
    "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
    "BNE", "CMP", "JAM", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
    "CPX", "SBC", "NOP", "ISC", "CPX", "SBC", "INC", "ISC", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC",
    "BEQ", "SBC", "JAM", "ISC", "NOP", "SBC", "INC", "ISC", "SED", "SBC", "NOP", "ISC", "NOP", "SBC", "INC", "ISC",
};

typedef struct {
    byte *prg;
    byte *chr;
    byte *sram;
    byte mapper;
    byte mirror;
    bool battery;
} Cartridge;

#define SAVE_RAM_CAP     0x2000
#define MEMORY_CAP       0x0800

#define ZERO_PAGE_OFFSET 0x0000
#define STACK_OFFSET     0x0100
#define RAM_OFFSET       0x0200
#define PRG_ROM_OFFSET   0x8000

typedef enum {
    ADDR_MODE_ABSOLUTE,
    ADDR_MODE_ABSOULUTE_X,
    ADDR_MODE_ABSOULUTE_Y,

    ADDR_MODE_ZERO_PAGE,
    ADDR_MODE_ZERO_PAGE_X,
    ADDR_MODE_ZERO_PAGE_Y,

    ADDR_MODE_INDIRECT,
    ADDR_MODE_INDIRECT_X,
    ADDR_MODE_INDIRECT_Y,

    ADDR_MODE_RELATIVE,
    ADDR_MODE_IMPLICIT,
    ADDR_MODE_IMMEDIATE,
    ADDR_MODE_ACCUMULATOR,
} Addressing_Mode;

typedef struct {
    // CPU
    u16  reg_pc; // Program counter
    byte reg_sp; // Stack pointer
    byte reg_a;  // Accumulator
    byte reg_x;
    byte reg_y;
    byte reg_p;  // Processor status
    Addressing_Mode addr_mode;
    byte mem[MEMORY_CAP];
    byte io_regs_one[8];
    byte io_regs_two[32];

    // Cartridge
    byte *prg;
    byte *chr;
    byte sram[SAVE_RAM_CAP];
    byte mapper;
    byte mirror;
    bool battery;
} NES;

void nes_exec_next_instruction(NES *nes) {
    (void) nes;
}

byte nes_read(const NES *nes, u16 address) {
    if (address > PRG_ROM_OFFSET && address < 0xFFFF) {
        return nes->prg[address - PRG_ROM_OFFSET];
    }

    fprintf(stderr, "Error: not able to read memory from address 0x%x\n", address);
    exit(1);
}

void nes_write(NES *nes, u16 address, byte byte) {
    (void) nes;
    (void) address;
    (void) byte;
}

void read_bytes(void *buffer, size_t count, FILE *stream) {
    size_t read = fread(buffer, sizeof(char), count, stream);
    if (read != count) {
        fprintf(stderr, "Error: cound not read %zu byte(s) from buffer\n", count);
        exit(1);
    }
}

byte read_byte(FILE *stream) {
    byte byte;
    read_bytes(&byte, 1, stream);
    return byte;
}

NES nes_from_ines_file(const char *filepath, Arena *arena) {
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

    byte prg_units = read_byte(file);
    byte chr_units = read_byte(file);

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

    size_t prg_size = prg_units * 16384;
    size_t chr_size = chr_units * 8192;

    byte *prg = arena_alloc(arena, prg_size);
    byte *chr = arena_alloc(arena, chr_size);

    read_bytes(prg, prg_size, file);
    read_bytes(chr, chr_size, file);

    fclose(file);

    byte mapper = (lower_nybble_mapper >> 4) | upper_nybble_mapper;
    byte mirroring = mirroring_arrangement | four_screen_vram;

    NES nes = { 0 };

    // Initialize cartridge data
    nes.prg = prg;
    nes.chr = chr;
    nes.mapper = mapper;
    nes.mirror = mirroring;
    nes.battery = contains_battery_backed_ram;

    // Initialize CPU data
    nes.reg_pc = nes_read(&nes, 0xFFFD) * 256 + nes_read(&nes, 0xFFFC);
    nes.reg_p = 0x34; // Is this correct?

    printf("Register PC: 0x%x\n", nes.reg_pc);
    return nes;
}

int main(void) {
    Arena arena = { 0 };

    NES nes = nes_from_ines_file(FILEPATH, &arena);
    nes_exec_next_instruction(&nes);

    arena_free(&arena);
    return 0;
}
