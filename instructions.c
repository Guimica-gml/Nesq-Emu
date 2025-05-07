// This isn't exactly valid c because it depends on structs
// defined in the main.c file, but I didn't want this in the main file

const int instruction_modes[256] = {
     9, 7,  0, 7, 3, 3, 3, 3, 9, 11, 12, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
     0, 7,  0, 7, 3, 3, 3, 3, 9, 11, 12, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
     9, 7,  0, 7, 3, 3, 3, 3, 9, 11, 12, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
     9, 7,  0, 7, 3, 3, 3, 3, 9, 11, 12, 11, 6, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
    11, 7, 11, 7, 3, 3, 3, 3, 9, 11,  9, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 5, 5, 9,  2,  9,  2, 1, 1, 2, 2,
    11, 7, 11, 7, 3, 3, 3, 3, 9, 11,  9, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 5, 5, 9,  2,  9,  2, 1, 1, 2, 2,
    11, 7, 11, 7, 3, 3, 3, 3, 9, 11,  9, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
    11, 7, 11, 7, 3, 3, 3, 3, 9, 11,  9, 11, 0, 0, 0, 0,
    10, 8,  0, 8, 4, 4, 4, 4, 9,  2,  9,  2, 1, 1, 1, 1,
};

const int instruction_sizes[256] = {
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

// TODO: implement as I write the instructions
const inst_func instruction_funcs[256] = {
    nes_brk, nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL,
    nes_bpl, nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL, NULL   , nes_ora, NULL   , NULL,
    nes_jsr, NULL   , NULL   , NULL, nes_bit, NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, nes_bit, NULL   , NULL   , NULL,
    NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL,
    NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, nes_pha, NULL   , NULL   , NULL, nes_jmp, NULL   , NULL   , NULL,
    NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL,
    nes_rts, nes_adc, NULL   , NULL, NULL   , nes_adc, NULL   , NULL, NULL   , nes_adc, NULL   , NULL, nes_jmp, nes_adc, NULL   , NULL,
    NULL   , nes_adc, NULL   , NULL, NULL   , nes_adc, NULL   , NULL, nes_sei, nes_adc, NULL   , NULL, NULL   , nes_adc, NULL   , NULL,
    NULL   , nes_sta, NULL   , NULL, NULL   , nes_sta, nes_stx, NULL, nes_dey, NULL   , NULL   , NULL, NULL   , nes_sta, nes_stx, NULL,
    NULL   , nes_sta, NULL   , NULL, NULL   , nes_sta, nes_stx, NULL, NULL   , nes_sta, nes_txs, NULL, NULL   , nes_sta, NULL   , NULL,
    nes_ldy, nes_lda, nes_ldx, NULL, nes_ldy, nes_lda, nes_ldx, NULL, NULL   , nes_lda, NULL   , NULL, nes_ldy, nes_lda, nes_ldx, NULL,
    nes_bcs, nes_lda, NULL   , NULL, nes_ldy, nes_lda, nes_ldx, NULL, NULL   , nes_lda, NULL   , NULL, nes_ldy, nes_lda, nes_ldx, NULL,
    nes_cpy, nes_cmp, NULL   , NULL, nes_cpy, nes_cmp, NULL   , NULL, nes_iny, nes_cmp, nes_dex, NULL, nes_cpy, nes_cmp, NULL   , NULL,
    nes_bne, nes_cmp, NULL   , NULL, NULL   , nes_cmp, NULL   , NULL, nes_cld, nes_cmp, NULL   , NULL, NULL   , nes_cmp, NULL   , NULL,
    nes_cpx, NULL   , NULL   , NULL, nes_cpx, NULL   , NULL   , NULL, nes_inx, NULL   , NULL   , NULL, nes_cpx, NULL   , NULL   , NULL,
    nes_beq, NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL, NULL   , NULL   , NULL   , NULL,
};

