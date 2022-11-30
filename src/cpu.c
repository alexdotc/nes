#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mem.h"
#include "util.h"

static inline uint8_t memread(CPU* restrict, uint16_t);

typedef union Operand{ /* mostly for fun since I never use them */
    uint8_t b1;
    uint16_t b2;
} Operand;

static inline void STA(CPU* restrict, Operand);
static inline void STX(CPU* restrict, Operand);
static inline void STY(CPU* restrict, Operand);
static inline void LDA(CPU* restrict, Operand);
static inline void LDX(CPU* restrict, Operand);
static inline void LDY(CPU* restrict, Operand);
static inline void CPX(CPU* restrict, Operand);
static inline void CPY(CPU* restrict, Operand);
static inline void CMP(CPU* restrict, Operand);
static inline void TAX(CPU* restrict, Operand);
static inline void TAY(CPU* restrict, Operand);
static inline void TSX(CPU* restrict, Operand);
static inline void TXA(CPU* restrict, Operand);
static inline void TXS(CPU* restrict, Operand);
static inline void TYA(CPU* restrict, Operand);
static inline void DEC(CPU* restrict, Operand);
static inline void INC(CPU* restrict, Operand);
static inline void INX(CPU* restrict, Operand);
static inline void INY(CPU* restrict, Operand);
static inline void DEX(CPU* restrict, Operand);
static inline void BRK(CPU* restrict, Operand);
static inline void BPL(CPU* restrict, Operand);
static inline void BIT(CPU* restrict, Operand);
static inline void BMI(CPU* restrict, Operand);
static inline void BCC(CPU* restrict, Operand);
static inline void BCS(CPU* restrict, Operand);
static inline void BVC(CPU* restrict, Operand);
static inline void BVS(CPU* restrict, Operand);
static inline void BEQ(CPU* restrict, Operand);
static inline void BNE(CPU* restrict, Operand);
static inline void ORA(CPU* restrict, Operand);
static inline void ASL(CPU* restrict, Operand);
static inline void DEY(CPU* restrict, Operand);
static inline void PLA(CPU* restrict, Operand);
static inline void PHA(CPU* restrict, Operand);
static inline void PHP(CPU* restrict, Operand);
static inline void PLP(CPU* restrict, Operand);
static inline void CLC(CPU* restrict, Operand);
static inline void CLD(CPU* restrict, Operand);
static inline void CLI(CPU* restrict, Operand);
static inline void CLV(CPU* restrict, Operand);
static inline void JMP(CPU* restrict, Operand);
static inline void JSR(CPU* restrict, Operand);
static inline void RTS(CPU* restrict, Operand);
static inline void RTI(CPU* restrict, Operand);
static inline void ROL(CPU* restrict, Operand);
static inline void SEC(CPU* restrict, Operand);
static inline void SEI(CPU* restrict, Operand);
static inline void AND(CPU* restrict, Operand);
static inline void ROR(CPU* restrict, Operand);
static inline void EOR(CPU* restrict, Operand);
static inline void LSR(CPU* restrict, Operand);
static inline void ADC(CPU* restrict, Operand);
static inline void SBC(CPU* restrict, Operand);
static inline void SED(CPU* restrict, Operand);
static inline void NOP(CPU* restrict, Operand);

/* addressing modes. Get operand based on register values and memory pointer */
static inline Operand addr_Accumulator(CPU* restrict);
static inline Operand addr_Absolute(CPU* restrict);
static inline Operand addr_AbsoluteX(CPU* restrict);
static inline Operand addr_AbsoluteY(CPU* restrict);
static inline Operand addr_Immediate(CPU* restrict);
static inline Operand addr_Implied(CPU* restrict);
static inline Operand addr_Indirect(CPU* restrict);
static inline Operand addr_IndirectX(CPU* restrict);
static inline Operand addr_IndirectY(CPU* restrict);
static inline Operand addr_Relative(CPU* restrict);
static inline Operand addr_ZeroPage(CPU* restrict);
static inline Operand addr_ZeroPageX(CPU* restrict);
static inline Operand addr_ZeroPageY(CPU* restrict);

/* array of function pointers to opcode routines, indexed by opcode number */
static const void (*opcodes[256])(CPU* restrict, Operand) =
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
#endif

/* array of function pointers to addressing modes indexed by opcode number */
static const Operand (*addrmodes[256])(CPU* restrict) =
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


CPU make_cpu(Memory* mem){
    uint8_t A = 0, X = 0, Y = 0;
    uint8_t P = STATUS_INIT;
    uint8_t SP = SP_INIT;
    uint16_t PC = 0;
    uint64_t cycles = STARTUP_CYCLES;
    CPU cpu = { cycles,A,X,Y,P,SP,PC,mem };

    return cpu;
}

static inline uint8_t memread(CPU* restrict cpu, uint16_t addr){
    uint8_t read = *(cpu->mem->map[addr]);
    return read;
}

static inline uint8_t memreadPC(CPU* restrict cpu){
    /* convenience wrapper to do a read at PC and then increment PC */
    uint16_t addr = cpu->PC;
    uint8_t read = memread(cpu, addr);
    cpu->PC++;
    return read;
}

void reset(CPU* restrict cpu){
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

void FDE(CPU* restrict cpu){
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
    /* TODO GET CYCLES and replace "42" */
    const char* mnemonic = mnemonic_str[opcode];
    fprintf(stdout, "%04X  %02X %02X %02X  %s $%02X%02X                       A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n", 
            _pc, opcode, _oper_low, _oper_high, mnemonic, _oper_high, _oper_low, _a, _x, _y, _p, _sp, cpu->cycles);
    #endif

    opcodes[opcode](cpu, op);
    cpu->cycles = cpu->cycles + cycles[opcode];

}

static inline Operand addr_Accumulator(CPU* restrict cpu){
    Operand op;
    op.b1 = 0;
    return op;
}

static inline Operand addr_Absolute(CPU* restrict cpu){
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);
    Operand op;
    op.b2 = (high << 8) | low;
    return op;
}

static inline Operand addr_AbsoluteX(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_AbsoluteY(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_Immediate(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_Implied(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_Indirect(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_IndirectX(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_IndirectY(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_Relative(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_ZeroPage(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_ZeroPageX(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline Operand addr_ZeroPageY(CPU* restrict cpu){
    Operand op;
    op.b1 =  0;
    return op;
}

static inline void STA(CPU* restrict cpu, Operand op){
    return;
}

static inline void STX(CPU* restrict cpu, Operand op){
    return;
}

static inline void STY(CPU* restrict cpu, Operand op){
    return;
}

static inline void LDA(CPU* restrict cpu, Operand op){
    return;
}

static inline void LDX(CPU* restrict cpu, Operand op){
    return;
}

static inline void LDY(CPU* restrict cpu, Operand op){
    return;
}

static inline void CPX(CPU* restrict cpu, Operand op){
    return;
}

static inline void CPY(CPU* restrict cpu, Operand op){
    return;
}

static inline void CMP(CPU* restrict cpu, Operand op){
    return;
}

static inline void TAX(CPU* restrict cpu, Operand op){
    return;
}

static inline void TAY(CPU* restrict cpu, Operand op){
    return;
}

static inline void TSX(CPU* restrict cpu, Operand op){
    return;
}

static inline void TXA(CPU* restrict cpu, Operand op){
    return;
}

static inline void TXS(CPU* restrict cpu, Operand op){
    return;
}

static inline void TYA(CPU* restrict cpu, Operand op){
    return;
}

static inline void DEC(CPU* restrict cpu, Operand op){
    return;
}

static inline void INC(CPU* restrict cpu, Operand op){
    return;
}

static inline void INX(CPU* restrict cpu, Operand op){
    return;
}

static inline void INY(CPU* restrict cpu, Operand op){
    return;
}

static inline void DEX(CPU* restrict cpu, Operand op){
    return;
}

static inline void BRK(CPU* restrict cpu, Operand op){
    return;
}

static inline void BPL(CPU* restrict cpu, Operand op){
    return;
}

static inline void BIT(CPU* restrict cpu, Operand op){
    return;
}

static inline void BMI(CPU* restrict cpu, Operand op){
    return;
}

static inline void BCC(CPU* restrict cpu, Operand op){
    return;
}

static inline void BCS(CPU* restrict cpu, Operand op){
    return;
}

static inline void BVC(CPU* restrict cpu, Operand op){
    return;
}

static inline void BVS(CPU* restrict cpu, Operand op){
    return;
}

static inline void BEQ(CPU* restrict cpu, Operand op){
    return;
}

static inline void BNE(CPU* restrict cpu, Operand op){
    return;
}

static inline void ORA(CPU* restrict cpu, Operand op){
    return;
}

static inline void ASL(CPU* restrict cpu, Operand op){
    return;
}

static inline void DEY(CPU* restrict cpu, Operand op){
    return;
}

static inline void PLA(CPU* restrict cpu, Operand op){
    return;
}

static inline void PHA(CPU* restrict cpu, Operand op){
    return;
}

static inline void PHP(CPU* restrict cpu, Operand op){
    return;
}

static inline void PLP(CPU* restrict cpu, Operand op){
    return;
}

static inline void CLC(CPU* restrict cpu, Operand op){
    return;
}

static inline void CLD(CPU* restrict cpu, Operand op){
    return;
}

static inline void CLI(CPU* restrict cpu, Operand op){
    return;
}

static inline void CLV(CPU* restrict cpu, Operand op){
    return;
}

static inline void JMP(CPU* restrict cpu, Operand op){
    cpu->PC = op.b2;
    return;
}

static inline void JSR(CPU* restrict cpu, Operand op){
    return;
}

static inline void RTS(CPU* restrict cpu, Operand op){
    return;
}

static inline void RTI(CPU* restrict cpu, Operand op){
    return;
}

static inline void ROL(CPU* restrict cpu, Operand op){
    return;
}

static inline void SEC(CPU* restrict cpu, Operand op){
    return;
}

static inline void SEI(CPU* restrict cpu, Operand op){
    return;
}

static inline void AND(CPU* restrict cpu, Operand op){
    return;
}

static inline void ROR(CPU* restrict cpu, Operand op){
    return;
}

static inline void EOR(CPU* restrict cpu, Operand op){
    return;
}

static inline void LSR(CPU* restrict cpu, Operand op){
    return;
}

static inline void ADC(CPU* restrict cpu, Operand op){
    return;
}

static inline void SBC(CPU* restrict cpu, Operand op){
    return;
}

static inline void SED(CPU* restrict cpu, Operand op){
    return;
}

static inline void NOP(CPU* restrict cpu, Operand op){
    return;
}
