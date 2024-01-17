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

#define bit_set(b, n)    (b) = ((b) | n)
#define bit_clear(b, n)  (b) = ((b) & ~n)
#define bit_toggle(b, n) (b) = ((b) ^ n)

#define bit_set_if(b, n, cond)                  \
    do {                                        \
        if (cond) {                             \
            bit_set(b, n);                      \
        } else {                                \
            bit_clear(b, n);                    \
        }                                       \
    } while (0);

// Useful for debugging, can't read hexadecimal
#define BYTE_FMT "%c%c%c%c%c%c%c%c"
#define BYTE_ARG(b)                      \
    ((b) & 0x80 ? '1' : '0'),            \
    ((b) & 0x40 ? '1' : '0'),            \
    ((b) & 0x20 ? '1' : '0'),            \
    ((b) & 0x10 ? '1' : '0'),            \
    ((b) & 0x08 ? '1' : '0'),            \
    ((b) & 0x04 ? '1' : '0'),            \
    ((b) & 0x02 ? '1' : '0'),            \
    ((b) & 0x01 ? '1' : '0')

typedef uint8_t  byte;
typedef uint16_t word;

typedef enum {
    ADDR_MODE_ABSOLUTE     = 0,
    ADDR_MODE_ABSOLUTE_X   = 1,
    ADDR_MODE_ABSOLUTE_Y   = 2,

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

typedef enum {
    STATUS_BIT_CARRY     = 1,
    STATUS_BIT_ZERO      = 2,
    STATUS_BIT_INTERRUPT = 4,
    STATUS_BIT_DECIMAL   = 8,
    STATUS_BIT_BREAK     = 16,
    STATUS_BIT_IGNORED   = 32,
    STATUS_BIT_OVERFLOW  = 64,
    STATUS_BIT_NEGATIVE  = 128,
} Status_Bit;

typedef struct {
    // CPU
    word reg_pc; // Program counter
    byte reg_sp; // Stack pointer
    byte reg_a;  // Accumulator
    byte reg_x;
    byte reg_y;
    byte reg_p;  // Processor status
    Addressing_Mode addr_mode;
    byte mem[0x800];
    byte io_regs_one[8];
    byte io_regs_two[32];

    // Cartridge
    byte *prg;
    byte *chr;
    byte sram[0x2000];
    byte mapper;
    byte mirror;
    bool battery;
} NES;

typedef int (*inst_func)(NES *nes, word address);

const char *addr_mode_name(Addressing_Mode addr_mode) {
    switch (addr_mode) {
        case ADDR_MODE_ABSOLUTE: return "abs";
        case ADDR_MODE_ABSOLUTE_X: return "abs,X";
        case ADDR_MODE_ABSOLUTE_Y: return "abs,Y";
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

word solve_mirroring(word address, word block_start, word block_size) {
    assert(address >= block_start);
    if (address < block_start + block_size) return address;
    word w = (address - block_start) % block_size;
    return address - (block_size * w);
}

byte nes_read(const NES *nes, word address) {
    if (address < 0x1FFF) {
        word a = solve_mirroring(address, 0x00, 0x800);
        return nes->mem[a];
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
        word a = solve_mirroring(address, 0x2000, 0x8);
        return nes->io_regs_one[a - 0x2000];
    }

    if (address >= 0x8000) {
        return nes->prg[address - 0x8000];
    }

    fprintf(stderr, "Error: not able to read memory from address 0x%X\n", address);
    exit(1);
}

void nes_write(NES *nes, word address, byte byte) {
    if (address <= 0x1FFF) {
        word a = solve_mirroring(address, 0x00, 0x800);
        nes->mem[a] = byte;
    } else if (address >= 0x2000 && address <= 0x3FFF) {
        word a = solve_mirroring(address, 0x2000, 0x8);
        nes->io_regs_one[a - 0x2000] = byte;
    } else {
        fprintf(stderr, "Error: not able to write to memory address 0x%X\n", address);
        exit(1);
    }
}

void nes_stack_push(NES *nes, byte byte) {
    assert(nes->reg_sp >= 0x00);
    nes->mem[0x100 + (nes->reg_sp--)] = byte;
}

byte nes_stack_pop(NES *nes) {
    assert(nes->reg_sp <= 0xFF);
    return nes->mem[0x100 + (++nes->reg_sp)];
}

void nes_stack_print(NES *nes) {
    size_t stack_size = 0xFF - nes->reg_sp;
    printf("Stack:\n");
    for (size_t i = 0; i < stack_size; ++i) {
        printf("0x%X\n", nes->mem[0x100 + 0xFF - i]);
    }
    printf("-----\n");
}

// Used by the instructions to get the address of memory where
// the data they need is located, can be written to or read from
word nes_fetch_data_address(NES *nes, word address) {
    switch (nes->addr_mode) {
        case ADDR_MODE_ABSOLUTE: {
            word lo = nes_read(nes, address + 1);
            word hi = nes_read(nes, address + 2);
            return lo | (hi << 8);
        }
        case ADDR_MODE_ABSOLUTE_X: {
            word lo = nes_read(nes, address + 1);
            word hi = nes_read(nes, address + 2);
            return (lo | (hi << 8)) + nes->reg_x;
        }
        case ADDR_MODE_ABSOLUTE_Y: {
            word lo = nes_read(nes, address + 1);
            word hi = nes_read(nes, address + 2);
            return (lo | (hi << 8)) + nes->reg_y;
        }
        case ADDR_MODE_ZERO_PAGE: {
            return nes_read(nes, address + 1);
        }
        case ADDR_MODE_ZERO_PAGE_X: {
            return nes_read(nes, address + 1) + nes->reg_x;
        }
        case ADDR_MODE_ZERO_PAGE_Y: {
            return nes_read(nes, address + 1) + nes->reg_y;
        }
        case ADDR_MODE_INDIRECT: {
            word lo = nes_read(nes, address + 1);
            // NOTE(nic): this is here to simulate a bug
            // in the indirect jmp (0x6C) instruction
            word hi;
            if (nes_read(nes, address) == 0x6C && (address & 0xFF) == 0xFF) {
                hi = nes_read(nes, address + 2);
            } else {
                hi = nes_read(nes, address + 1 - 0xFF);
            }
            word other = lo | (hi << 8);
            lo = nes_read(nes, other);
            hi = nes_read(nes, other + 1);
            return lo | (hi << 8);
        }
        case ADDR_MODE_INDIRECT_X: {
            word ind = nes_read(nes, address + 1);
            word a = ind + nes->reg_x;
            word lo = nes_read(nes, a);
            word hi = nes_read(nes, a + 1);
            return lo | (hi << 8);
        }
        case ADDR_MODE_INDIRECT_Y: {
            word a = nes_read(nes, address + 1);
            word lo = nes_read(nes, a);
            word hi = nes_read(nes, a + 1);
            return (lo | (hi << 8)) + nes->reg_y;
        }
        case ADDR_MODE_RELATIVE: {
            word rel = nes_read(nes, address + 1);
            return nes->reg_pc + rel;
        }
        case ADDR_MODE_IMPLIED: {
            fprintf(stderr, "Error: cannot fetch from addresing mode 'implied'\n");
            exit(1);
        }
        case ADDR_MODE_IMMEDIATE: {
            return address + 1;
        }
        case ADDR_MODE_ACCUMULATOR: {
            fprintf(stderr, "Error: cannot fetch from addresing mode 'accumulator'\n");
            exit(1);
        }
        default: {
            assert(0 && "unreachable");
        }
    }
}

bool nes_page_cross(word addr1, word addr2) {
    return (addr1 & 0xFF00) != (addr2 & 0xFF00);
}

typedef struct {
    Addressing_Mode items[5];
    size_t count;
} Addr_Mode_List;

#define mlist(c, ...) (Addr_Mode_List) { .items = {__VA_ARGS__}, .count = c }

// Listen, I know it's madness, but please, let this one slide, thank you and sorry
bool check_page_cross_x(NES *nes, word addr1, word addr2, Addr_Mode_List list) {
    for (size_t i = 0; i < list.count; ++i) {
        if (list.items[i] == nes->addr_mode) {
            return nes_page_cross(addr1, addr2);
        }
    }
    return false;
}

int nes_jmp(NES *nes, word address) {
    nes->reg_pc = nes_fetch_data_address(nes, address);
    return 0;
}

int nes_jsr(NES *nes, word address) {
    nes_stack_push(nes, (byte) ((address & 0xFF00) >> 8));
    nes_stack_push(nes, (byte) ((address & 0x00FF) >> 0));
    nes->reg_pc = nes_fetch_data_address(nes, address);
    return 0;
}

int nes_rts(NES *nes, word address) {
    (void) address;
    word lo = nes_stack_pop(nes);
    word hi = nes_stack_pop(nes);
    nes->reg_pc = lo | (hi << 8);
    return 0;
}

int nes_sei(NES *nes, word address) {
    (void) address;
    bit_set(nes->reg_p, STATUS_BIT_INTERRUPT);
    return 0;
}

int nes_cld(NES *nes, word address) {
    (void) address;
    bit_clear(nes->reg_p, STATUS_BIT_DECIMAL);
    return 0;
}

int nes_lda(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    nes->reg_a = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_a == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_a & 0x80) != 0);

    return check_page_cross_x(
        nes,
        address,
        data_addr,
        mlist(3, ADDR_MODE_ABSOLUTE_X, ADDR_MODE_ABSOLUTE_Y, ADDR_MODE_INDIRECT_Y));
}

int nes_ldx(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    nes->reg_x = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_x == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_x & 0x80) != 0);

    return check_page_cross_x(
        nes,
        address,
        data_addr,
        mlist(1, ADDR_MODE_ABSOLUTE_Y));
}

int nes_ldy(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    nes->reg_y = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_y == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_y & 0x80) != 0);

    return check_page_cross_x(
        nes,
        address,
        data_addr,
        mlist(1, ADDR_MODE_ABSOLUTE_X));
}

int nes_sta(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    nes_write(nes, data_addr, nes->reg_a);
    return 0;
}

int nes_stx(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    nes_write(nes, data_addr, nes->reg_x);
    return 0;
}

int nes_txs(NES *nes, word address) {
    (void) address;
    nes_write(nes, 0x100 + ((word) nes->reg_sp), nes->reg_x);
    return 0;
}

int nes_bpl(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    if ((nes->reg_p & STATUS_BIT_NEGATIVE) != 0) {
        nes->reg_pc = data_addr;
    }
    return nes_page_cross(address, data_addr) ? 2 : 1;
}

int nes_bcs(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    if ((nes->reg_p & STATUS_BIT_CARRY) != 0) {
        nes->reg_pc = data_addr;
    }
    return nes_page_cross(address, data_addr) ? 2 : 1;
}

int nes_bne(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    if ((nes->reg_p & STATUS_BIT_ZERO) == 0) {
        nes->reg_pc = data_addr;
    }
    return nes_page_cross(address, data_addr) ? 2 : 1;
}

int nes_cmp(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    byte data = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_CARRY, nes->reg_a >= data);
    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_a == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_a & 0x80) != 0);

    return check_page_cross_x(
        nes,
        address,
        data_addr,
        mlist(3, ADDR_MODE_ABSOLUTE_X, ADDR_MODE_ABSOLUTE_Y, ADDR_MODE_INDIRECT_Y));
}

int nes_cpx(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    byte data = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_CARRY, nes-> reg_x >= data);
    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_x == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_x & 0x80) != 0);
    return 0;
}

int nes_cpy(NES *nes, word address) {
    word data_addr = nes_fetch_data_address(nes, address);
    byte data = nes_read(nes, data_addr);

    bit_set_if(nes->reg_p, STATUS_BIT_CARRY, nes-> reg_y >= data);
    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_y == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_y & 0x80) != 0);
    return 0;
}

int nes_dex(NES *nes, word address) {
    (void) address;
    nes->reg_x -= 1;

    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_x == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_x & 0x80) != 0);
    return 0;
}

int nes_dey(NES *nes, word address) {
    (void) address;
    nes->reg_y -= 1;

    bit_set_if(nes->reg_p, STATUS_BIT_ZERO, nes->reg_y == 0);
    bit_set_if(nes->reg_p, STATUS_BIT_NEGATIVE, (nes->reg_y & 0x80) != 0);
    return 0;
}

#include "./instructions.def"

void nes_exec_next_instruction(NES *nes) {
    word inst_addr = nes->reg_pc;
    byte opcode = nes_read(nes, inst_addr);
    byte add_cycles = 0;

    nes->reg_pc += instruction_sizes[opcode];
    nes->addr_mode = instruction_modes[opcode];

    printf("Inst addr: 0x%X\n", inst_addr);
    printf("Op code: 0x%X\n", opcode);
    printf(
        "Inst name: %s %s\n",
        instruction_names[opcode],
        addr_mode_name(nes->addr_mode));
    printf("Inst size: %d\n", instruction_sizes[opcode]);

    if (instruction_funcs[opcode]) {
        add_cycles = instruction_funcs[opcode](nes, inst_addr);
    } else {
        fprintf(
            stderr, "Error: instruction `%s` is not implemented\n",
            instruction_names[opcode]);
        exit(1);
    }

    printf("Inst cycles: %d (+%d)\n", instruction_cycles[opcode], add_cycles);
    printf("--------------\n");
    nes_stack_print(nes);
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
    }

    arena_free(&arena);
    return 0;
}
