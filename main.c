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
    ADDR_MODE_ABSOLUTE     = 0,
    ADDR_MODE_ABSOULUTE_X  = 1,
    ADDR_MODE_ABSOULUTE_Y  = 2,

    ADDR_MODE_ZERO_PAGE    = 3,
    ADDR_MODE_ZERO_PAGE_X  = 4,
    ADDR_MODE_ZERO_PAGE_Y  = 5,

    ADDR_MODE_INDIRECT     = 6,
    ADDR_MODE_INDIRECT_X   = 7,
    ADDR_MODE_INDIRECT_Y   = 8,

    ADDR_MODE_IMPLIED      = 9,
    ADDR_MODE_RELATIVE     = 10,
    ADDR_MODE_IMMEDIATE    = 11,
    ADDR_MODE_ACCUMULATOR  = 12,
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

typedef size_t (*inst_func)(NES *nes, u16 address);

const char *addr_mode_name(Addressing_Mode addr_mode) {
    switch (addr_mode) {
        case ADDR_MODE_ABSOLUTE: return "abs";
        case ADDR_MODE_ABSOULUTE_X: return "abs,X";
        case ADDR_MODE_ABSOULUTE_Y: return "abs,Y";
        case ADDR_MODE_ZERO_PAGE: return "zpg";
        case ADDR_MODE_ZERO_PAGE_X: return "xpg,X";
        case ADDR_MODE_ZERO_PAGE_Y: return "xpg,Y";
        case ADDR_MODE_INDIRECT: return "ind";
        case ADDR_MODE_INDIRECT_X: return "X,ind";
        case ADDR_MODE_INDIRECT_Y: return "ind,Y";
        case ADDR_MODE_RELATIVE: return "rel";
        case ADDR_MODE_IMPLIED: return "imp";
        case ADDR_MODE_IMMEDIATE: return "#";
        case ADDR_MODE_ACCUMULATOR: return "A";
        default: {
            assert(0 && "unreachable");
        }
    }
}

byte nes_read(const NES *nes, u16 address) {
    if (address >= PRG_ROM_OFFSET) {
        return nes->prg[address - PRG_ROM_OFFSET];
    }

    fprintf(stderr, "Error: not able to read memory from address 0x%x\n", address);
    exit(1);
}

void nes_write(NES *nes, u16 address, byte byte) {
    (void) nes;
    (void) address;
    (void) byte;
    assert(0 && "unimplemented");
}

void nes_stack_push(NES *nes, byte byte) {
    assert(nes->reg_sp >= 0x00);
    nes->mem[STACK_OFFSET + (nes->reg_sp--)] = byte;
}

byte nes_stack_pop(NES *nes) {
    assert(nes->reg_sp <= 0xFF);
    return nes->mem[STACK_OFFSET + (++nes->reg_sp)];
}

void nes_stack_print(NES *nes) {
    size_t stack_size = 0xFF - nes->reg_sp;
    printf("Stack:\n");
    for (size_t i = 0; i < stack_size; ++i) {
        printf("0x%X\n", nes->mem[STACK_OFFSET + 0xFF - i]);
    }
    printf("-----\n");
}

// NOTE(nic): the nes jmp instruction has a bug
// I have to simulate that bug, look into that later
size_t nes_exec_jmp(NES *nes, u16 address) {
    if (nes->addr_mode == ADDR_MODE_ABSOLUTE) {
        u16 lo = nes_read(nes, address + 1);
        u16 hi = nes_read(nes, address + 2);
        nes->reg_pc = lo | (hi << 8);
    } else if (nes->addr_mode == ADDR_MODE_INDIRECT) {
        assert(0 && "unimplemented");
    } else {
        assert(0 && "unreachable");
    }
    return 0;
}

size_t nes_exec_jsr(NES *nes, u16 address) {
    assert(nes->addr_mode == ADDR_MODE_ABSOLUTE);
    nes_stack_push(nes, (byte) ((address & 0xFF00) >> 8));
    nes_stack_push(nes, (byte) (address & 0x00FF));
    u16 lo = nes_read(nes, address + 1);
    u16 hi = nes_read(nes, address + 2);
    nes->reg_pc = lo | (hi << 8);
    return 0;
}

size_t nes_exec_rts(NES *nes, u16 address) {
    (void) address;
    assert(nes->addr_mode == ADDR_MODE_IMPLIED);
    u16 lo = nes_stack_pop(nes);
    u16 hi = nes_stack_pop(nes);
    nes->reg_pc = lo | (hi << 8);
    return 0;
}

#include "./instructions.def"

void nes_exec_next_instruction(NES *nes) {
    u16 inst_addr = nes->reg_pc;
    byte opcode = nes_read(nes, inst_addr);
    nes->reg_pc += instruction_sizes[opcode];

    nes->addr_mode = instruction_modes[opcode];
    if (instruction_funcs[opcode]) {
        instruction_funcs[opcode](nes, inst_addr);
    }

    printf("Inst addr: 0x%X\n", inst_addr);
    printf("Op code: 0x%X\n", opcode);
    printf("Inst name: %s %s\n",
           instruction_names[opcode],
           addr_mode_name(nes->addr_mode));
    printf("Inst cycles: %d\n", instruction_cycles[opcode]);
    printf("Inst size: %zu\n", instruction_sizes[opcode]);
    printf("--------------\n");
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
    nes.reg_sp = 0xFF;
    nes.reg_p = 0x34; // Is this correct?

    return nes;
}

int main(void) {
    Arena arena = { 0 };

    NES nes = nes_from_ines_file(FILEPATH, &arena);

    for (size_t i = 0; i < 100; ++i) {
        nes_exec_next_instruction(&nes);
        nes_stack_print(&nes);
    }

    arena_free(&arena);
    return 0;
}
