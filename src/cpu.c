#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "mem.h"
#include "util.h"

static uint8_t memread(CPU*, uint16_t);
static void memwrite(CPU*, uint16_t, uint8_t);

typedef union Operand{ /* mostly for fun since I never use them */
    uint8_t b1;
    uint16_t b2;
} Operand;

static void STA(CPU*, Operand);
static void STX(CPU*, Operand);
static void STY(CPU*, Operand);
static void LDA(CPU*, Operand);
static void LDX(CPU*, Operand);
static void LDY(CPU*, Operand);
static void CPX(CPU*, Operand);
static void CPY(CPU*, Operand);
static void CMP(CPU*, Operand);
static void TAX(CPU*, Operand);
static void TAY(CPU*, Operand);
static void TSX(CPU*, Operand);
static void TXA(CPU*, Operand);
static void TXS(CPU*, Operand);
static void TYA(CPU*, Operand);
static void DEC(CPU*, Operand);
static void INC(CPU*, Operand);
static void INX(CPU*, Operand);
static void INY(CPU*, Operand);
static void DEX(CPU*, Operand);
static void BRK(CPU*, Operand);
static void BPL(CPU*, Operand);
static void BIT(CPU*, Operand);
static void BMI(CPU*, Operand);
static void BCC(CPU*, Operand);
static void BCS(CPU*, Operand);
static void BVC(CPU*, Operand);
static void BVS(CPU*, Operand);
static void BEQ(CPU*, Operand);
static void BNE(CPU*, Operand);
static void ORA(CPU*, Operand);
static void ASL(CPU*, Operand);
static void DEY(CPU*, Operand);
static void PLA(CPU*, Operand);
static void PHA(CPU*, Operand);
static void PHP(CPU*, Operand);
static void PLP(CPU*, Operand);
static void CLC(CPU*, Operand);
static void CLD(CPU*, Operand);
static void CLI(CPU*, Operand);
static void CLV(CPU*, Operand);
static void JMP(CPU*, Operand);
static void JSR(CPU*, Operand);
static void RTS(CPU*, Operand);
static void RTI(CPU*, Operand);
static void ROL(CPU*, Operand);
static void SEC(CPU*, Operand);
static void SEI(CPU*, Operand);
static void AND(CPU*, Operand);
static void ROR(CPU*, Operand);
static void EOR(CPU*, Operand);
static void LSR(CPU*, Operand);
static void ADC(CPU*, Operand);
static void SBC(CPU*, Operand);
static void SED(CPU*, Operand);
static void NOP(CPU*, Operand);

/* addressing modes. Get operand based on register values and memory pointer */
static Operand addr_Accumulator(CPU*);
static Operand addr_Absolute(CPU*);
static Operand addr_AbsoluteX(CPU*);
static Operand addr_AbsoluteY(CPU*);
static Operand addr_Immediate(CPU*);
static Operand addr_Implied(CPU*);
static Operand addr_Indirect(CPU*);
static Operand addr_IndirectX(CPU*);
static Operand addr_IndirectY(CPU*);
static Operand addr_Relative(CPU*);
static Operand addr_ZeroPage(CPU*);
static Operand addr_ZeroPageX(CPU*);
static Operand addr_ZeroPageY(CPU*);

/* array of function pointers to opcode routines, indexed by opcode number */
static const void (*opcodes[256])(CPU*, Operand) =
{
    BRK, ORA, NULL, NULL, NULL, ORA, ASL, NULL, PHP, ORA, ASL, NULL, NULL, ORA, ASL, NULL, /* 00-OF */
    BPL, ORA, NULL, NULL, NULL, ORA, ASL, NULL, CLC, ORA, NULL, NULL, NULL, ORA, ASL, NULL, /* 10-1F */
    JSR, AND, NULL, NULL, BIT, AND, ROL, NULL, PLP, AND, ROL, NULL, BIT, AND, ROL, NULL, /* 20-2F */
    BMI, AND, NULL, NULL, NULL, AND, ROL, NULL, SEC, AND, NULL, NULL, NULL, AND, ROL, NULL, /* 30-3F */
    RTI, EOR, NULL, NULL, NULL, EOR, LSR, NULL, PHA, EOR, LSR, NULL, JMP, EOR, LSR, NULL, /* 40-4F */
    BVC, EOR, NULL, NULL, NULL, EOR, LSR, NULL, CLI, EOR, NULL, NULL, NULL, EOR, LSR, NULL, /* 50-5F */
    RTS, ADC, NULL, NULL, NULL, ADC, ROR, NULL, PLA, ADC, ROR, NULL, JMP, ADC, ROR, NULL, /* 60-6F */
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
static const Operand (*addrmodes[256])(CPU*) =
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
    uint8_t read = *(cpu->mem->map[addr]);
    return read;
}

static void memwrite(CPU* cpu, uint16_t addr, uint8_t val){
    *(cpu->mem->map[addr]) = val;
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
        err_exit("fatal: CPU: Illegal opcode %02x at location %04X\n", opcode, cpu->PC-1);
    Operand op;
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
            fprintf(stdout, "%04X  %02X        %s                                 A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
                    _pc, opcode, mnemonic, _a, _x, _y, _p, _sp, cpu->cycles);
            break;
        case 1:
            fprintf(stdout, "%04X  %02X %02X     %s #$%02X                           A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
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

static Operand addr_Accumulator(CPU* cpu){
    /* nothing */
    Operand op;
    op.b1 = 0;
    return op;
}

static Operand addr_Absolute(CPU* cpu){
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);
    Operand op;
    op.b2 = (high << 8) | low;
    return op;
}

static Operand addr_AbsoluteX(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_AbsoluteY(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_Immediate(CPU* cpu){
    Operand op;
    op.b1 = memreadPC(cpu);
    return op;
}

static Operand addr_Implied(CPU* cpu){
    /* nothing */
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_Indirect(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_IndirectX(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_IndirectY(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_Relative(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_ZeroPage(CPU* cpu){
    uint16_t low = memreadPC(cpu);
    Operand op;
    op.b2 =  low;
    return op;
}

static Operand addr_ZeroPageX(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static Operand addr_ZeroPageY(CPU* cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static void STA(CPU* cpu, Operand op){
    /* store A at address op */
    memwrite(cpu, op.b2, cpu->A);
    return;
}

static void STX(CPU* cpu, Operand op){
    /* store X at address op */
    memwrite(cpu, op.b2, cpu->X);
    return;
}

static void STY(CPU* cpu, Operand op){
    /* store Y at address op */
    memwrite(cpu, op.b2, cpu->Y);
    return;
}

static void LDA(CPU* cpu, Operand op){
    cpu->A = op.b1;
    return;
}

static void LDX(CPU* cpu, Operand op){
    cpu->X = op.b1;
    return;
}

static void LDY(CPU* cpu, Operand op){
    cpu->Y = op.b1;
    return;
}

static void CPX(CPU* cpu, Operand op){
    return;
}

static void CPY(CPU* cpu, Operand op){
    return;
}

static void CMP(CPU* cpu, Operand op){
    return;
}

static void TAX(CPU* cpu, Operand op){
    return;
}

static void TAY(CPU* cpu, Operand op){
    return;
}

static void TSX(CPU* cpu, Operand op){
    return;
}

static void TXA(CPU* cpu, Operand op){
    return;
}

static void TXS(CPU* cpu, Operand op){
    return;
}

static void TYA(CPU* cpu, Operand op){
    return;
}

static void DEC(CPU* cpu, Operand op){
    return;
}

static void INC(CPU* cpu, Operand op){
    return;
}

static void INX(CPU* cpu, Operand op){
    return;
}

static void INY(CPU* cpu, Operand op){
    return;
}

static void DEX(CPU* cpu, Operand op){
    return;
}

static void BRK(CPU* cpu, Operand op){
    return;
}

static void BPL(CPU* cpu, Operand op){
    return;
}

static void BIT(CPU* cpu, Operand op){
    return;
}

static void BMI(CPU* cpu, Operand op){
    return;
}

static void BCC(CPU* cpu, Operand op){
    return;
}

static void BCS(CPU* cpu, Operand op){
    return;
}

static void BVC(CPU* cpu, Operand op){
    return;
}

static void BVS(CPU* cpu, Operand op){
    return;
}

static void BEQ(CPU* cpu, Operand op){
    return;
}

static void BNE(CPU* cpu, Operand op){
    return;
}

static void ORA(CPU* cpu, Operand op){
    return;
}

static void ASL(CPU* cpu, Operand op){
    return;
}

static void DEY(CPU* cpu, Operand op){
    return;
}

static void PLA(CPU* cpu, Operand op){
    return;
}

static void PHA(CPU* cpu, Operand op){
    return;
}

static void PHP(CPU* cpu, Operand op){
    return;
}

static void PLP(CPU* cpu, Operand op){
    return;
}

static void CLC(CPU* cpu, Operand op){
    return;
}

static void CLD(CPU* cpu, Operand op){
    return;
}

static void CLI(CPU* cpu, Operand op){
    return;
}

static void CLV(CPU* cpu, Operand op){
    return;
}

static void JMP(CPU* cpu, Operand op){
    cpu->PC = op.b2;
    return;
}

static void JSR(CPU* cpu, Operand op){
    return;
}

static void RTS(CPU* cpu, Operand op){
    return;
}

static void RTI(CPU* cpu, Operand op){
    return;
}

static void ROL(CPU* cpu, Operand op){
    return;
}

static void SEC(CPU* cpu, Operand op){
    return;
}

static void SEI(CPU* cpu, Operand op){
    return;
}

static void AND(CPU* cpu, Operand op){
    return;
}

static void ROR(CPU* cpu, Operand op){
    return;
}

static void EOR(CPU* cpu, Operand op){
    return;
}

static void LSR(CPU* cpu, Operand op){
    return;
}

static void ADC(CPU* cpu, Operand op){
    return;
}

static void SBC(CPU* cpu, Operand op){
    return;
}

static void SED(CPU* cpu, Operand op){
    return;
}

static void NOP(CPU* cpu, Operand op){
    return;
}
