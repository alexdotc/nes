#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "mem.h"
#include "util.h"

static uint8_t memread(CPU*, uint16_t);
static void memwrite(CPU*, uint16_t, uint8_t);

static void stack_push(CPU*, uint8_t);
static uint8_t stack_pull(CPU*);

typedef enum flags{C = 0, I = 2, D = 3, V = 6} flags; 
static void set_flag(flags, CPU*, bool);
static void update_Z(CPU*, uint8_t);
static void update_N(CPU*, int8_t);

static void check_pagecross(CPU*, uint16_t);

static void branch(bool, CPU*, uint16_t);
static void compare(CPU*, uint8_t, uint16_t);
static void load(CPU*, uint8_t*, uint16_t);
static void xbc(CPU*, uint16_t, bool);

static void STA(CPU*, uint16_t);
static void STX(CPU*, uint16_t);
static void STY(CPU*, uint16_t);
static void LDA(CPU*, uint16_t);
static void LDX(CPU*, uint16_t);
static void LDY(CPU*, uint16_t);
static void CPX(CPU*, uint16_t);
static void CPY(CPU*, uint16_t);
static void CMP(CPU*, uint16_t);
static void TAX(CPU*, uint16_t);
static void TAY(CPU*, uint16_t);
static void TSX(CPU*, uint16_t);
static void TXA(CPU*, uint16_t);
static void TXS(CPU*, uint16_t);
static void TYA(CPU*, uint16_t);
static void DEC(CPU*, uint16_t);
static void INC(CPU*, uint16_t);
static void INX(CPU*, uint16_t);
static void INY(CPU*, uint16_t);
static void DEX(CPU*, uint16_t);
static void BRK(CPU*, uint16_t);
static void BPL(CPU*, uint16_t);
static void BIT(CPU*, uint16_t);
static void BMI(CPU*, uint16_t);
static void BCC(CPU*, uint16_t);
static void BCS(CPU*, uint16_t);
static void BVC(CPU*, uint16_t);
static void BVS(CPU*, uint16_t);
static void BEQ(CPU*, uint16_t);
static void BNE(CPU*, uint16_t);
static void ORA(CPU*, uint16_t);
static void DEY(CPU*, uint16_t);
static void PLA(CPU*, uint16_t);
static void PHA(CPU*, uint16_t);
static void PHP(CPU*, uint16_t);
static void PLP(CPU*, uint16_t);
static void CLC(CPU*, uint16_t);
static void CLD(CPU*, uint16_t);
static void CLI(CPU*, uint16_t);
static void CLV(CPU*, uint16_t);
static void JMP(CPU*, uint16_t);
static void JSR(CPU*, uint16_t);
static void RTS(CPU*, uint16_t);
static void RTI(CPU*, uint16_t);
static void SEC(CPU*, uint16_t);
static void SEI(CPU*, uint16_t);
static void AND(CPU*, uint16_t);
static void EOR(CPU*, uint16_t);
static void ADC(CPU*, uint16_t);
static void SBC(CPU*, uint16_t);
static void SED(CPU*, uint16_t);
static void NOP(CPU*, uint16_t);
static void ASL(CPU*, uint16_t);
static void LSR(CPU*, uint16_t);
static void ROL(CPU*, uint16_t);
static void ROR(CPU*, uint16_t);
static void ASL_A(CPU*, uint16_t);
static void LSR_A(CPU*, uint16_t);
static void ROL_A(CPU*, uint16_t);
static void ROR_A(CPU*, uint16_t);

/* addressing modes. Get operand based on register values and memory pointer */
static uint16_t addr_Accumulator(CPU*);
static uint16_t addr_Absolute(CPU*);
static uint16_t addr_AbsoluteX(CPU*);
static uint16_t addr_AbsoluteY(CPU*);
static uint16_t addr_Immediate(CPU*);
static uint16_t addr_Implied(CPU*);
static uint16_t addr_Indirect(CPU*);
static uint16_t addr_IndirectX(CPU*);
static uint16_t addr_IndirectY(CPU*);
static uint16_t addr_Relative(CPU*);
static uint16_t addr_ZeroPage(CPU*);
static uint16_t addr_ZeroPageX(CPU*);
static uint16_t addr_ZeroPageY(CPU*);

/* array of function pointers to opcode routines, indexed by opcode number */
static const void (*opcodes[256])(CPU*, uint16_t) =
{
    BRK, ORA, NULL, NULL, NULL, ORA, ASL, NULL, PHP, ORA, ASL_A, NULL, NULL, ORA, ASL, NULL, /* 00-OF */
    BPL, ORA, NULL, NULL, NULL, ORA, ASL, NULL, CLC, ORA, NULL, NULL, NULL, ORA, ASL, NULL, /* 10-1F */
    JSR, AND, NULL, NULL, BIT, AND, ROL, NULL, PLP, AND, ROL_A, NULL, BIT, AND, ROL, NULL, /* 20-2F */
    BMI, AND, NULL, NULL, NULL, AND, ROL, NULL, SEC, AND, NULL, NULL, NULL, AND, ROL, NULL, /* 30-3F */
    RTI, EOR, NULL, NULL, NULL, EOR, LSR, NULL, PHA, EOR, LSR_A, NULL, JMP, EOR, LSR, NULL, /* 40-4F */
    BVC, EOR, NULL, NULL, NULL, EOR, LSR, NULL, CLI, EOR, NULL, NULL, NULL, EOR, LSR, NULL, /* 50-5F */
    RTS, ADC, NULL, NULL, NULL, ADC, ROR, NULL, PLA, ADC, ROR_A, NULL, JMP, ADC, ROR, NULL, /* 60-6F */
    BVS, ADC, NULL, NULL, NULL, ADC, ROR, NULL, SEI, ADC, NULL, NULL, NULL, ADC, ROR, NULL, /* 70-7F */
    NULL, STA, NULL, NULL, STY, STA, STX, NULL, DEY, NULL, TXA, NULL, STY, STA, STX, NULL, /* 80-8F */
    BCC, STA, NULL, NULL, STY, STA, STX, NULL, TYA, STA, TXS, NULL, NULL, STA, NULL, NULL, /* 90-9F */
    LDY, LDA, LDX, NULL, LDY, LDA, LDX, NULL, TAY, LDA, TAX, NULL, LDY, LDA, LDX, NULL, /* A0-AF */
    BCS, LDA, NULL, NULL, LDY, LDA, LDX, NULL, CLV, LDA, TSX, NULL, LDY, LDA, LDX, NULL, /* B0-BF */
    CPY, CMP, NULL, NULL, CPY, CMP, DEC, NULL, INY, CMP, DEX, NULL, CPY, CMP, DEC, NULL, /* C0-CF */
    BNE, CMP, NULL, NULL, NULL, CMP, DEC, NULL, CLD, CMP, NULL, NULL, NULL, CMP, DEC, NULL, /* D0-DF */
    CPX, SBC, NULL, NULL, CPX, SBC, INC, NULL, INX, SBC, NOP, NULL, CPX, SBC, INC, NULL, /* E0-EF */
    BEQ, SBC, NULL, NULL, NULL, SBC, INC, NULL, SED, SBC, NULL, NULL, NULL, SBC, INC, NULL /* F0-FF */
};

/* base number of cycles per instruction (can be +1 or +2 depending on whether page boundaries are 
   crossed, and, in the case of branch instructions, whether or not the branch was taken) 
   6502 Illegal opcodes are just 0 to keep things simple. We die if we see one for now, anyway */
static const uint8_t cycles[256] =
{
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, /* 00-OF */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, /* 10-1F */
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, /* 20-2F */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, /* 30-3F */
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0, /* 40-4F */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, /* 50-5F */
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, /* 60-6F */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, /* 70-7F */
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, /* 80-8F */
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, /* 90-9F */
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, /* A0-AF */
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0, /* B0-BF */
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, /* C0-CF */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, /* D0-DF */
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, /* E0-EF */
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0 /* F0-FF */
};

/* array of function pointers to addressing modes indexed by opcode number */
static const uint16_t (*addrmodes[256])(CPU*) =
{
    addr_Implied, addr_Indirect, NULL, NULL, NULL, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Accumulator, NULL, NULL, addr_Absolute, addr_Absolute, NULL, /* 00-OF */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL, /* 10-1F */
    addr_Absolute, addr_IndirectX, NULL, NULL, addr_ZeroPage, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Accumulator, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* 20-2F */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL, /* 30-3F */
    addr_Implied, addr_IndirectX, NULL, NULL, NULL, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Accumulator, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* 40-4F */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL, /* 50-5F */
    addr_Implied, addr_IndirectX, NULL, NULL, NULL, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Accumulator, NULL, addr_Indirect, addr_Absolute, addr_Absolute, NULL, /* 60-6F */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL, /* 70-7F */
    NULL, addr_IndirectX, NULL, NULL, addr_ZeroPage, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, NULL, addr_Implied, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* 80-8F */
    addr_Relative, addr_IndirectY, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, addr_ZeroPageY, NULL, addr_Implied, addr_AbsoluteY, addr_Implied, NULL, NULL, addr_AbsoluteX, NULL, NULL, /* 90-9F */
    addr_Immediate, addr_IndirectX, addr_Immediate, NULL, addr_ZeroPage, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Implied, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* A0-AF */
    addr_Relative, addr_IndirectY, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, addr_ZeroPageY, NULL, addr_Implied, addr_AbsoluteY, addr_Implied, NULL, addr_AbsoluteX, addr_AbsoluteX, addr_AbsoluteY, NULL, /* B0-BF */
    addr_Immediate, addr_IndirectX, NULL, NULL, addr_ZeroPage, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Implied, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* C0-CF */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL, /* D0-DF */
    addr_Immediate, addr_IndirectX, NULL, NULL, addr_ZeroPage, addr_ZeroPage, addr_ZeroPage, NULL, addr_Implied, addr_Immediate, addr_Implied, NULL, addr_Absolute, addr_Absolute, addr_Absolute, NULL, /* E0-EF */
    addr_Relative, addr_IndirectY, NULL, NULL, NULL, addr_ZeroPageX, addr_ZeroPageX, NULL, addr_Implied, addr_AbsoluteY, NULL, NULL, NULL, addr_AbsoluteX, addr_AbsoluteX, NULL /* F0-FF */
};

#ifdef DEBUG
static const char* mnemonic_str[256] =
{
    "BRK", "ORA", NULL, NULL, NULL, "ORA", "ASL", NULL, "PHP", "ORA", "ASL", NULL, NULL, "ORA", "ASL", NULL, /* 00-OF */
    "BPL", "ORA", NULL, NULL, NULL, "ORA", "ASL", NULL, "CLC", "ORA", NULL, NULL, NULL, "ORA", "ASL", NULL, /* 10-1F */
    "JSR", "AND", NULL, NULL, "BIT", "AND", "ROL", NULL, "PLP", "AND", "ROL", NULL, "BIT", "AND", "ROL", NULL, /* 20-2F */
    "BMI", "AND", NULL, NULL, NULL, "AND", "ROL", NULL, "SEC", "AND", NULL, NULL, NULL, "AND", "ROL", NULL, /* 30-3F */
    "RTI", "EOR", NULL, NULL, NULL, "EOR", "LSR", NULL, "PHA", "EOR", "LSR", NULL, "JMP", "EOR", "LSR", NULL, /* 40-4F */
    "BVC", "EOR", NULL, NULL, NULL, "EOR", "LSR", NULL, "CLI", "EOR", NULL, NULL, NULL, "EOR", "LSR", NULL, /* 50-5F */
    "RTS", "ADC", NULL, NULL, NULL, "ADC", "ROR", NULL, "PLA", "ADC", "ROR", NULL, "JMP", "ADC", "ROR", NULL, /* 60-6F */
    "BVS", "ADC", NULL, NULL, NULL, "ADC", "ROR", NULL, "SEI", "ADC", NULL, NULL, NULL, "ADC", "ROR", NULL, /* 70-7F */
    NULL, "STA", NULL, NULL, "STY", "STA", "STX", NULL, "DEY", NULL, "TXA", NULL, "STY", "STA", "STX", NULL, /* 80-8F */
    "BCC", "STA", NULL, NULL, "STY", "STA", "STX", NULL, "TYA", "STA", "TXS", NULL, NULL, "STA", NULL, NULL, /* 90-9F */
    "LDY", "LDA", "LDX", NULL, "LDY", "LDA", "LDX", NULL, "TAY", "LDA", "TAX", NULL, "LDY", "LDA", "LDX", NULL, /* A0-AF */
    "BCS", "LDA", NULL, NULL, "LDY", "LDA", "LDX", NULL, "CLV", "LDA", "TSX", NULL, "LDY", "LDA", "LDX", NULL, /* B0-BF */
    "CPY", "CMP", NULL, NULL, "CPY", "CMP", "DEC", NULL, "INY", "CMP", "DEX", NULL, "CPY", "CMP", "DEC", NULL, /* C0-CF */
    "BNE", "CMP", NULL, NULL, NULL, "CMP", "DEC", NULL, "CLD", "CMP", NULL, NULL, NULL, "CMP", "DEC", NULL, /* D0-DF */
    "CPX", "SBC", NULL, NULL, "CPX", "SBC", "INC", NULL, "INX", "SBC", "NOP", NULL, "CPX", "SBC", "INC", NULL, /* E0-EF */
    "BEQ", "SBC", NULL, NULL, NULL, "SBC", "INC", NULL, "SED", "SBC", NULL, NULL, NULL, "SBC", "INC", NULL /* F0-FF */
};

static const char* addr_string[256] = 
{
    "Implied", "Indirect", NULL, NULL, NULL, "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Accumulator", NULL, NULL, "Absolute", "Absolute", NULL, /* 00-OF */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL, /* 10-1F */
    "Absolute", "IndirectX", NULL, NULL, "ZeroPage", "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Accumulator", NULL, "Absolute", "Absolute", "Absolute", NULL, /* 20-2F */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL, /* 30-3F */
    "Implied", "IndirectX", NULL, NULL, NULL, "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Accumulator", NULL, "Absolute", "Absolute", "Absolute", NULL, /* 40-4F */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL, /* 50-5F */
    "Implied", "IndirectX", NULL, NULL, NULL, "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Accumulator", NULL, "Indirect", "Absolute", "Absolute", NULL, /* 60-6F */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL, /* 70-7F */
    NULL, "IndirectX", NULL, NULL, "ZeroPage", "ZeroPage", "ZeroPage", NULL, "Implied", NULL, "Implied", NULL, "Absolute", "Absolute", "Absolute", NULL, /* 80-8F */
    "Relative", "IndirectY", NULL, NULL, "ZeroPageX", "ZeroPageX", "ZeroPageY", NULL, "Implied", "AbsoluteY", "Implied", NULL, NULL, "AbsoluteX", NULL, NULL, /* 90-9F */
    "Immediate", "IndirectX", "Immediate", NULL, "ZeroPage", "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Implied", NULL, "Absolute", "Absolute", "Absolute", NULL, /* A0-AF */
    "Relative", "IndirectY", NULL, NULL, "ZeroPageX", "ZeroPageX", "ZeroPageY", NULL, "Implied", "AbsoluteY", "Implied", NULL, "AbsoluteX", "AbsoluteX", "AbsoluteY", NULL, /* B0-BF */
    "Immediate", "IndirectX", NULL, NULL, "ZeroPage", "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Implied", NULL, "Absolute", "Absolute", "Absolute", NULL, /* C0-CF */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL, /* D0-DF */
    "Immediate", "IndirectX", NULL, NULL, "ZeroPage", "ZeroPage", "ZeroPage", NULL, "Implied", "Immediate", "Implied", NULL, "Absolute", "Absolute", "Absolute", NULL, /* E0-EF */
    "Relative", "IndirectY", NULL, NULL, NULL, "ZeroPageX", "ZeroPageX", NULL, "Implied", "AbsoluteY", NULL, NULL, NULL, "AbsoluteX", "AbsoluteX", NULL /* F0-FF */
};
#endif


CPU make_cpu(Memory* mem){
    uint8_t A = 0, X = 0, Y = 0;
    uint8_t P = STATUS_INIT;
    uint8_t SP = SP_INIT;
    uint16_t PC = 0;
    uint64_t cycles = STARTUP_CYCLES;
    CPU cpu = { cycles,A,X,Y,P,SP,PC,mem };

    return cpu;
}

static uint8_t memread(CPU* cpu, uint16_t addr){
    return *(cpu->mem->map[addr]);
}

static void memwrite(CPU* cpu, uint16_t addr, uint8_t val){
    /* TODO Guard writable range on map (can't write rom...) */
    *(cpu->mem->map[addr]) = val;
}

static void stack_push(CPU* cpu, uint8_t val){
    memwrite(cpu, STACK_BOTTOM + cpu->SP--, val);
}

static uint8_t stack_pull(CPU* cpu){
    return memread(cpu, STACK_BOTTOM + ++cpu->SP);
}

static uint8_t memreadPC(CPU* cpu){
    /* convenience wrapper to do a read at PC and then increment PC */
    uint16_t addr = cpu->PC;
    uint8_t read = memread(cpu, addr);
    cpu->PC++;
    return read;
}

void reset(CPU* cpu){
    /* set PC to address in reset vector */
    cpu->PC = RESET;
    cpu->cycles = STARTUP_CYCLES;
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);

    cpu->PC = (high << 8) | low; /* jump */
    #ifdef DEBUG
    cpu->PC = 0xC000; /* nestest first instruction */
    #endif
}

void FDE(CPU* cpu){
    /* cpu main Fetch-Decode-Execute loop */
    #ifdef DEBUG /* snapshot pre-instruction state (put this in function? TODO)*/
    uint16_t _pc = cpu->PC;
    uint8_t _a  = cpu->A;
    uint8_t _x  = cpu->X;
    uint8_t _y  = cpu->Y;
    uint8_t _p  = cpu->P;
    uint8_t _sp  = cpu->SP;
    uint8_t _oper_low = memread(cpu, _pc+1);
    uint8_t _oper_high = memread(cpu, _pc+2);
    #endif

    uint8_t opcode = memreadPC(cpu); 
    if (addrmodes[opcode] == NULL)
        err_exit("CPU: Illegal opcode %02x at location %04X", opcode, cpu->PC-1);
    uint16_t op;
    op = addrmodes[opcode](cpu); /* TODO figure out how to handle 0 or 1 byte operands cleanly and in debug */

    #ifdef DEBUG
    /* TODO write to a log file or stdout */
    const char* mnemonic = mnemonic_str[opcode];
    const char* addrstring = addr_string[opcode];
    uint8_t n_oper;

    if (strncmp(addrstring, "Implied", 3) == 0) n_oper = 0;
    else if (strncmp(addrstring, "Accumulator", 2) == 0) n_oper = 0;
    else if (strcmp(addrstring, "AbsoluteX") == 0) n_oper = 2;
    else if (strcmp(addrstring, "AbsoluteY") == 0) n_oper = 2;
    else if (strncmp(addrstring, "Absolute", 2) == 0) n_oper = 2;
    else if (strcmp(addrstring, "Indirect") == 0) n_oper = 2;
    else n_oper = 1;

    switch(n_oper){
        case 0:
            fprintf(stdout, "%04X  %02X        %s                             A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
                    _pc, opcode, mnemonic, _a, _x, _y, _p, _sp, cpu->cycles);
            break;
        case 1:
            fprintf(stdout, "%04X  %02X %02X     %s #$%02X                        A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
                    _pc, opcode, _oper_low, mnemonic, _oper_low, _a, _x, _y, _p, _sp, cpu->cycles);
            break;
        case 2:
            fprintf(stdout, "%04X  %02X %02X %02X  %s $%02X%02X                       A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
                    _pc, opcode, _oper_low, _oper_high, mnemonic, _oper_high, _oper_low, _a, _x, _y, _p, _sp, cpu->cycles);
    }
    #endif

    opcodes[opcode](cpu, op);

    cpu->cycles = cpu->cycles + cycles[opcode];

}

static void check_pagecross(CPU* cpu, uint16_t target){
    /* if high byte different */
    if ((target & 0xFF00) != (cpu->PC & 0xFF00))
        cpu->cycles++;
}

static void set_flag(flags bitpos, CPU* cpu, bool set){
    /* set bitpos of cpu's status register to bool set */
    if (set)
        cpu->P = (1 << bitpos) | cpu->P;
    else
        cpu->P = ~(1 << bitpos) & cpu->P;
}

static void update_Z(CPU* cpu, uint8_t res){
    /* set Z if res is 0, clear otherwise */
    if (res == 0)
        cpu->P = (1 << 1) | cpu->P;
    else
        cpu->P = ~(1 << 1) & cpu->P;
}

static void update_N(CPU* cpu, int8_t res){
    /* set N if res is negative, clear otherwise */
    if (res < 0)
        cpu->P = (1 << 7) | cpu->P;
    else
        cpu->P = ~(1 << 7) & cpu->P;
}

static uint16_t addr_Accumulator(CPU* cpu){
    /* nop */
    return 0;
}

static uint16_t addr_Absolute(CPU* cpu){
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);
    uint16_t addr = (high << 8) | low;
    return addr;
}

static uint16_t addr_AbsoluteX(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode AbsoluteX at location %04X", cpu->PC-1);
    return 0;
}

static uint16_t addr_AbsoluteY(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode AbsoluteY at location %04X", cpu->PC-1);
    return 0;
}

static uint16_t addr_Immediate(CPU* cpu){
    /* simply return the PRGROM address in the PC, which is the
     * address of the immediate byte (incremented after opcode
     * byte is read). We expect the instruction function to treat 
     * as an address and "dereference" the actual immediate value */
    return cpu->PC++;
}

static uint16_t addr_Implied(CPU* cpu){
    /* nop */
    return 0;
}

static uint16_t addr_Indirect(CPU* cpu){
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);
    uint16_t addr = (high << 8) | low;
    low = memread(cpu, addr);
    high = memread(cpu, addr+1);
    addr = (high << 8) | low;
    return addr;
}

static uint16_t addr_IndirectX(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode IndirectX at location %04X", cpu->PC-1);
    return 0;
}

static uint16_t addr_IndirectY(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode IndirectY at location %04X", cpu->PC-1);
    return 0;
}

static uint16_t addr_Relative(CPU* cpu){
    /* twos complement signed byte */
    int8_t rel = memreadPC(cpu);
    uint16_t addr = ((int16_t)cpu->PC) + rel;
    return addr;
}

static uint16_t addr_ZeroPage(CPU* cpu){
    /* just read the low byte and use $00 as the high byte */
    uint16_t addr = memreadPC(cpu);
    return addr;
}

static uint16_t addr_ZeroPageX(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode ZeroPageX at location %04X", cpu->PC-1);
    return 0;
}

static uint16_t addr_ZeroPageY(CPU* cpu){
    err_exit("CPU: Unimplemented addressing mode ZeroPageY at location %04X", cpu->PC-1);
    return 0;
}

static void branch(bool branch_taken, CPU* cpu, uint16_t op){
    if (!branch_taken)
        return;
    /* +1 cycle if page crossed in branch */
    check_pagecross(cpu, op);
    cpu->PC = op;
    /* +1 cycle when branch taken */
    cpu->cycles++;
}

static void load(CPU* cpu, uint8_t* reg, uint16_t op){
    *reg = memread(cpu, op);
    update_Z(cpu, *reg);
    update_N(cpu, *reg);
}

static void compare(CPU* cpu, uint8_t other, uint16_t op){
    uint8_t cmp = other - memread(cpu, op);
    update_N(cpu, cmp);
    update_Z(cpu, cmp);
    set_flag(C, cpu, cmp <= other);
}

static void xbc(CPU* cpu, uint16_t op, bool invert){
    /* core logic for ADC and SBC. if invert is true, do SBC. else do ADC.
       these instructions perform addition or subtraction of the form
       Accumulator +- Operand +- Carry bit (bit 0 of the status register), where
       the carry bit is inverted for a subtract (the 'borrow flag').
       
       The overflow flag is set when we consider this as a signed operation and
       interpret our operands and result as two's complement values.
       
       The carry flag is still set as per usual, when we have "unsigned"
       (hardware) overflow (a carry in the highest order bit). This dual
       interpreation and dual meaning of the term "overflow" makes this a bit
       tricky.

       In the case of subtraction, we can simply take the ones' complement
       (invert the bits) of the second op and do addition for the same effect */
    uint8_t read = memread(cpu, op);
    uint8_t carry_in = cpu->P & 1;
    if (invert) read = ~read;
    uint8_t res = carry_in + read + cpu->A;
    /* read and accum both have different signs than res.
       signed addition overflowed */
    bool overflow = ((read >> 7) ^ (res >> 7)) & ((cpu->A >> 7) ^ (res >> 7));
    /* unsigned addition overflowed */
    bool carry_out = (res < read || (res == read && carry_in == 1));
    cpu->A = res;
    set_flag(V, cpu, overflow);
    set_flag(C, cpu, carry_out);
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void STA(CPU* cpu, uint16_t op){
    memwrite(cpu, op, cpu->A);
}

static void STX(CPU* cpu, uint16_t op){
    memwrite(cpu, op, cpu->X);
}

static void STY(CPU* cpu, uint16_t op){
    memwrite(cpu, op, cpu->Y);
}

static void LDA(CPU* cpu, uint16_t op){
    load(cpu, &cpu->A, op);
}

static void LDX(CPU* cpu, uint16_t op){
    load(cpu, &cpu->X, op);
}

static void LDY(CPU* cpu, uint16_t op){
    load(cpu, &cpu->Y, op);
}

static void CMP(CPU* cpu, uint16_t op){
    compare(cpu, cpu->A, op);
}

static void CPX(CPU* cpu, uint16_t op){
    compare(cpu, cpu->X, op);
}

static void CPY(CPU* cpu, uint16_t op){
    compare(cpu, cpu->Y, op);
}

static void TSX(CPU* cpu, uint16_t op){
    cpu->X = cpu->SP;
    update_Z(cpu, cpu->X);
    update_N(cpu, cpu->X);
}

static void TXS(CPU* cpu, uint16_t op){
    cpu->SP = cpu->X;
}

static void TAX(CPU* cpu, uint16_t op){
    cpu->X = cpu->A;
    update_Z(cpu, cpu->X);
    update_N(cpu, cpu->X);
}

static void TAY(CPU* cpu, uint16_t op){
    cpu->Y = cpu->A;
    update_Z(cpu, cpu->Y);
    update_N(cpu, cpu->Y);
}

static void TXA(CPU* cpu, uint16_t op){
    cpu->A = cpu->X;
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void TYA(CPU* cpu, uint16_t op){
    cpu->A = cpu->Y;
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void INC(CPU* cpu, uint16_t op){
    uint8_t read = memread(cpu, op);
    read++;
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void INX(CPU* cpu, uint16_t op){
    cpu->X++;
    update_Z(cpu, cpu->X);
    update_N(cpu, cpu->X);
}

static void INY(CPU* cpu, uint16_t op){
    cpu->Y++;
    update_Z(cpu, cpu->Y);
    update_N(cpu, cpu->Y);
}

static void DEC(CPU* cpu, uint16_t op){
    uint8_t read = memread(cpu, op);
    read--;
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void DEX(CPU* cpu, uint16_t op){
    cpu->X--;
    update_Z(cpu, cpu->X);
    update_N(cpu, cpu->X);
}

static void DEY(CPU* cpu, uint16_t op){
    cpu->Y--;
    update_Z(cpu, cpu->Y);
    update_N(cpu, cpu->Y);
}

static void BIT(CPU* cpu, uint16_t op){
    uint8_t read = memread(cpu, op);
    update_Z(cpu, read & cpu->A);
    update_N(cpu, read);
    set_flag(V, cpu, read & (1 << 6));
}

static void BPL(CPU* cpu, uint16_t op){
    branch(((1 << 7) & cpu->P) == 0, cpu, op);
}

static void BMI(CPU* cpu, uint16_t op){
    branch(((1 << 7) & cpu->P) != 0, cpu, op);
}

static void BCC(CPU* cpu, uint16_t op){
    branch((1 & cpu->P) == 0, cpu, op);
}

static void BCS(CPU* cpu, uint16_t op){
    branch((1 & cpu->P) != 0, cpu, op);
}

static void BVC(CPU* cpu, uint16_t op){
    branch(((1 << 6) & cpu->P) == 0, cpu, op);
}

static void BVS(CPU* cpu, uint16_t op){
    branch(((1 << 6) & cpu->P) != 0, cpu, op);
}

static void BEQ(CPU* cpu, uint16_t op){
    branch(((1 << 1) & cpu->P) != 0, cpu, op);
}

static void BNE(CPU* cpu, uint16_t op){
    branch(((1 << 1) & cpu->P) == 0, cpu, op);
}

static void PLA(CPU* cpu, uint16_t op){
    cpu->A = stack_pull(cpu);
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void PHA(CPU* cpu, uint16_t op){
    stack_push(cpu, cpu->A);
}

static void PHP(CPU* cpu, uint16_t op){
    /* status register value is pushed to stack with bits 4 and 5 set
       Note that bit 5 should always be set for convenience in our case
       since it doesn't exist in real hardware */
    stack_push(cpu, cpu->P | (3 << 4));
}

static void PLP(CPU* cpu, uint16_t op){
    /* status register value is pulled from stack with
       bit 4 clear. Note that bit 5 should always be set for
       convenience in our case since it doesn't exist in real hardware */
    cpu->P = stack_pull(cpu) & ~(1 << 4);
    cpu->P |= (1 << 5);

}

static void CLC(CPU* cpu, uint16_t op){
    set_flag(C, cpu, false);
}

static void CLD(CPU* cpu, uint16_t op){
    set_flag(D, cpu, false);
}

static void CLI(CPU* cpu, uint16_t op){
    set_flag(I, cpu, false);
}

static void CLV(CPU* cpu, uint16_t op){
    set_flag(V, cpu, false);
}

static void JMP(CPU* cpu, uint16_t op){
    cpu->PC = op;
}

static void JSR(CPU* cpu, uint16_t op){
    uint8_t low = 0x00FF & cpu->PC;
    uint8_t high = cpu->PC >> 8;
    /* adjust off-by-one which usually won't matter
       except in very specific sequences where a stack pull
       happens before an RTS */
    if (low == 0) high--; /* overflow */
    low--;
    stack_push(cpu, high);
    stack_push(cpu, low);
    cpu->PC = op;
}

static void RTS(CPU* cpu, uint16_t op){
    uint8_t low = stack_pull(cpu);
    uint8_t high = stack_pull(cpu);
    cpu->PC = ((uint16_t) high << 8) | low;
    /* adjust off-by-one which usually won't matter
       except in very specific sequences */
    cpu->PC++;
}

static void RTI(CPU* cpu, uint16_t op){
    /* equivalent to PLP */
    cpu->P = stack_pull(cpu) & ~(1 << 4);
    cpu->P |= (1 << 5);
    /* subtle difference from RTS in that the PC
       will be 1 less: the actual address pulled
       from stack */
    uint8_t low = stack_pull(cpu);
    uint8_t high = stack_pull(cpu);
    cpu->PC = ((uint16_t) high << 8) | low;
}

static void SEC(CPU* cpu, uint16_t op){
    set_flag(C, cpu, true);
}

static void SEI(CPU* cpu, uint16_t op){
    set_flag(I, cpu, true);
}

static void AND(CPU* cpu, uint16_t op){
    cpu->A &= memread(cpu, op);
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void EOR(CPU* cpu, uint16_t op){
    cpu->A ^= memread(cpu, op);
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void ORA(CPU* cpu, uint16_t op){
    cpu->A |= memread(cpu, op);
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void ADC(CPU* cpu, uint16_t op){
    xbc(cpu, op, false);
}

static void SBC(CPU* cpu, uint16_t op){
    xbc(cpu, op, true);
}

static void SED(CPU* cpu, uint16_t op){
    set_flag(D, cpu, true);
}

static void ASL(CPU* cpu, uint16_t op){
    uint8_t read = memread(cpu, op);
    set_flag(C, cpu, read & 0x80);
    read <<= 1; /* not flip bind lol */
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void LSR(CPU* cpu, uint16_t op){
    uint8_t read = memread(cpu, op);
    set_flag(C, cpu, read & 1);
    read >>= 1; /* not bind lol */
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void ROL(CPU* cpu, uint16_t op){
    /* left "rotate", except (post-rotation) bit 0 becomes the old
     * carry and the new carry becomes the bit that was rotated out
     * (pre-rotation bit 7). in other words, the operand is not actually
     * rotated about itself. very misleading use of the term "rotate"... */
    uint8_t read = memread(cpu, op);
    bool save_carry = cpu->P & 0x80;
    set_flag(C, cpu, read & 0x80);
    read <<= 1;
    if (save_carry)
        read |= 1;
    else
        read &= ~1;
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void ROR(CPU* cpu, uint16_t op){
    /* right "rotate", except (post-rotation) bit 7 becomes the old
     * carry and the new carry becomes the bit that was rotated out
     * (pre-rotation bit 0). in other words, the operand is not actually
     * rotated about itself. very misleading use of the term "rotate"... */
    uint8_t read = memread(cpu, op);
    bool save_carry = cpu->P & 1;
    set_flag(C, cpu, read & 1);
    read >>= 1;
    if (save_carry)
        read |= 0x80;
    else
        read &= ~0x80;
    memwrite(cpu, op, read);
    update_Z(cpu, read);
    update_N(cpu, read);
}

static void ASL_A(CPU* cpu, uint16_t op){
    set_flag(C, cpu, cpu->A & 0x80);
    cpu->A <<= 1; /* not flip bind lol */
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void LSR_A(CPU* cpu, uint16_t op){
    set_flag(C, cpu, cpu->A & 1);
    cpu->A >>= 1;
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void ROL_A(CPU* cpu, uint16_t op){
    bool save_carry = cpu->P & 0x80;
    set_flag(C, cpu, cpu->A & 0x80);
    cpu->A <<= 1;
    if (save_carry)
        cpu->A |= 1;
    else
        cpu->A &= ~1;
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void ROR_A(CPU* cpu, uint16_t op){
    bool save_carry = cpu->P & 1;
    set_flag(C, cpu, cpu->A & 1);
    cpu->A >>= 1;
    if (save_carry)
        cpu->A |= 0x80;
    else
        cpu->A &= ~0x80;
    update_Z(cpu, cpu->A);
    update_N(cpu, cpu->A);
}

static void BRK(CPU* cpu, uint16_t op){
    return;
}

static void NOP(CPU* cpu, uint16_t op){
    return;
}
