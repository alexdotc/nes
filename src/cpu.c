#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mem.h"
#include "util.h"

static inline uint8_t memread(CPU*, uint16_t);

static inline void STA(CPU*, uint16_t);
static inline void STX(CPU*, uint16_t);
static inline void STY(CPU*, uint16_t);
static inline void LDA(CPU*, uint16_t);
static inline void LDX(CPU*, uint16_t);
static inline void LDY(CPU*, uint16_t);
static inline void CPX(CPU*, uint16_t);
static inline void CPY(CPU*, uint16_t);
static inline void CMP(CPU*, uint16_t);
static inline void TAX(CPU*, uint16_t);
static inline void TAY(CPU*, uint16_t);
static inline void TSX(CPU*, uint16_t);
static inline void TXA(CPU*, uint16_t);
static inline void TXS(CPU*, uint16_t);
static inline void TYA(CPU*, uint16_t);
static inline void DEC(CPU*, uint16_t);
static inline void INC(CPU*, uint16_t);
static inline void INX(CPU*, uint16_t);
static inline void INY(CPU*, uint16_t);
static inline void DEX(CPU*, uint16_t);
static inline void BRK(CPU*, uint16_t);
static inline void BPL(CPU*, uint16_t);
static inline void BIT(CPU*, uint16_t);
static inline void BMI(CPU*, uint16_t);
static inline void BCC(CPU*, uint16_t);
static inline void BCS(CPU*, uint16_t);
static inline void BVC(CPU*, uint16_t);
static inline void BVS(CPU*, uint16_t);
static inline void BEQ(CPU*, uint16_t);
static inline void BNE(CPU*, uint16_t);
static inline void ORA(CPU*, uint16_t);
static inline void ASL(CPU*, uint16_t);
static inline void DEY(CPU*, uint16_t);
static inline void PLA(CPU*, uint16_t);
static inline void PHA(CPU*, uint16_t);
static inline void PHP(CPU*, uint16_t);
static inline void PLP(CPU*, uint16_t);
static inline void CLC(CPU*, uint16_t);
static inline void CLD(CPU*, uint16_t);
static inline void CLI(CPU*, uint16_t);
static inline void CLV(CPU*, uint16_t);
static inline void JMP(CPU*, uint16_t);
static inline void JSR(CPU*, uint16_t);
static inline void RTS(CPU*, uint16_t);
static inline void RTI(CPU*, uint16_t);
static inline void ROL(CPU*, uint16_t);
static inline void SEC(CPU*, uint16_t);
static inline void SEI(CPU*, uint16_t);
static inline void AND(CPU*, uint16_t);
static inline void ROR(CPU*, uint16_t);
static inline void EOR(CPU*, uint16_t);
static inline void LSR(CPU*, uint16_t);
static inline void ADC(CPU*, uint16_t);
static inline void SBC(CPU*, uint16_t);
static inline void SED(CPU*, uint16_t);
static inline void NOP(CPU*, uint16_t);

/* addressing modes. Get operand based on register values and memory pointer */
static inline uint16_t addr_Accumulator(CPU*);
static inline uint16_t addr_Absolute(CPU*);
static inline uint16_t addr_AbsoluteX(CPU*);
static inline uint16_t addr_AbsoluteY(CPU*);
static inline uint16_t addr_Immediate(CPU*);
static inline uint16_t addr_Implied(CPU*);
static inline uint16_t addr_Indirect(CPU*);
static inline uint16_t addr_IndirectX(CPU*);
static inline uint16_t addr_IndirectY(CPU*);
static inline uint16_t addr_Relative(CPU*);
static inline uint16_t addr_ZeroPage(CPU*);
static inline uint16_t addr_ZeroPageX(CPU*);
static inline uint16_t addr_ZeroPageY(CPU*);

/* array of function pointers to opcode routines, indexed by opcode number */
static const void (*opcodes[256])(CPU*, uint16_t) =
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


CPU make_cpu(Memory* mem){
    uint8_t A = 0, X = 0, Y = 0;
    uint8_t P = STATUS_INIT;
    uint8_t SP = SP_INIT;
    uint16_t PC = 0;

    CPU cpu = { A,X,Y,P,SP,PC,mem };

    return cpu;
}

static inline uint8_t memread(CPU* cpu, uint16_t addr){
    uint8_t read = *(cpu->mem->map[addr]);
    return read;
}

static inline uint8_t memreadPC(CPU* cpu){
    /* convenience wrapper to do a read at PC and then increment PC */
    uint16_t addr = cpu->PC;
    uint8_t read = memread(cpu, addr);
    cpu->PC++;
    return read;
}

void reset(CPU* cpu){
    /* set PC to address in reset vector */
    cpu->PC = RESET;
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);

    cpu->PC = (high << 8) | low; /* jump */
    #ifdef DEBUG
    cpu->PC = 0xC000; /* nestest log */
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
    uint16_t operand = addrmodes[opcode](cpu); /* TODO figure out how to handle 0 or 1 byte operands cleanly and in debug */

    #ifdef DEBUG
    /* TODO write to a log file or stdout */
    /* TODO GET CYCLES and replace "42" */
    const char* mnemonic = mnemonic_str[opcode];
    fprintf(stdout, "%04X  %02X %02X %02X  %s $%02X%02X                       A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", 
            _pc, opcode, _oper_low, _oper_high, mnemonic, _oper_high, _oper_low, _a, _x, _y, _p, _sp, 42);
    #endif

    opcodes[opcode](cpu, operand);

}

static inline uint16_t addr_Accumulator(CPU* cpu){
    return 0;
}

static inline uint16_t addr_Absolute(CPU* cpu){
    uint16_t low = memreadPC(cpu);
    uint16_t high = memreadPC(cpu);
    uint16_t full = (high << 8) | low;
    return full;
}

static inline uint16_t addr_AbsoluteX(CPU* cpu){
    return 0;
}

static inline uint16_t addr_AbsoluteY(CPU* cpu){
    return 0;
}

static inline uint16_t addr_Immediate(CPU* cpu){
    return 0;
}

static inline uint16_t addr_Implied(CPU* cpu){
    return 0;
}

static inline uint16_t addr_Indirect(CPU* cpu){
    return 0;
}

static inline uint16_t addr_IndirectX(CPU* cpu){
    return 0;
}

static inline uint16_t addr_IndirectY(CPU* cpu){
    return 0;
}

static inline uint16_t addr_Relative(CPU* cpu){
    return 0;
}

static inline uint16_t addr_ZeroPage(CPU* cpu){
    return 0;
}

static inline uint16_t addr_ZeroPageX(CPU* cpu){
    return 0;
}

static inline uint16_t addr_ZeroPageY(CPU* cpu){
    return 0;
}

static inline void STA(CPU* cpu, uint16_t operand){
    return;
}

static inline void STX(CPU* cpu, uint16_t operand){
    return;
}

static inline void STY(CPU* cpu, uint16_t operand){
    return;
}

static inline void LDA(CPU* cpu, uint16_t operand){
    return;
}

static inline void LDX(CPU* cpu, uint16_t operand){
    return;
}

static inline void LDY(CPU* cpu, uint16_t operand){
    return;
}

static inline void CPX(CPU* cpu, uint16_t operand){
    return;
}

static inline void CPY(CPU* cpu, uint16_t operand){
    return;
}

static inline void CMP(CPU* cpu, uint16_t operand){
    return;
}

static inline void TAX(CPU* cpu, uint16_t operand){
    return;
}

static inline void TAY(CPU* cpu, uint16_t operand){
    return;
}

static inline void TSX(CPU* cpu, uint16_t operand){
    return;
}

static inline void TXA(CPU* cpu, uint16_t operand){
    return;
}

static inline void TXS(CPU* cpu, uint16_t operand){
    return;
}

static inline void TYA(CPU* cpu, uint16_t operand){
    return;
}

static inline void DEC(CPU* cpu, uint16_t operand){
    return;
}

static inline void INC(CPU* cpu, uint16_t operand){
    return;
}

static inline void INX(CPU* cpu, uint16_t operand){
    return;
}

static inline void INY(CPU* cpu, uint16_t operand){
    return;
}

static inline void DEX(CPU* cpu, uint16_t operand){
    return;
}

static inline void BRK(CPU* cpu, uint16_t operand){
    return;
}

static inline void BPL(CPU* cpu, uint16_t operand){
    return;
}

static inline void BIT(CPU* cpu, uint16_t operand){
    return;
}

static inline void BMI(CPU* cpu, uint16_t operand){
    return;
}

static inline void BCC(CPU* cpu, uint16_t operand){
    return;
}

static inline void BCS(CPU* cpu, uint16_t operand){
    return;
}

static inline void BVC(CPU* cpu, uint16_t operand){
    return;
}

static inline void BVS(CPU* cpu, uint16_t operand){
    return;
}

static inline void BEQ(CPU* cpu, uint16_t operand){
    return;
}

static inline void BNE(CPU* cpu, uint16_t operand){
    return;
}

static inline void ORA(CPU* cpu, uint16_t operand){
    return;
}

static inline void ASL(CPU* cpu, uint16_t operand){
    return;
}

static inline void DEY(CPU* cpu, uint16_t operand){
    return;
}

static inline void PLA(CPU* cpu, uint16_t operand){
    return;
}

static inline void PHA(CPU* cpu, uint16_t operand){
    return;
}

static inline void PHP(CPU* cpu, uint16_t operand){
    return;
}

static inline void PLP(CPU* cpu, uint16_t operand){
    return;
}

static inline void CLC(CPU* cpu, uint16_t operand){
    return;
}

static inline void CLD(CPU* cpu, uint16_t operand){
    return;
}

static inline void CLI(CPU* cpu, uint16_t operand){
    return;
}

static inline void CLV(CPU* cpu, uint16_t operand){
    return;
}

static inline void JMP(CPU* cpu, uint16_t operand){
    cpu->PC = operand;
    return;
}

static inline void JSR(CPU* cpu, uint16_t operand){
    return;
}

static inline void RTS(CPU* cpu, uint16_t operand){
    return;
}

static inline void RTI(CPU* cpu, uint16_t operand){
    return;
}

static inline void ROL(CPU* cpu, uint16_t operand){
    return;
}

static inline void SEC(CPU* cpu, uint16_t operand){
    return;
}

static inline void SEI(CPU* cpu, uint16_t operand){
    return;
}

static inline void AND(CPU* cpu, uint16_t operand){
    return;
}

static inline void ROR(CPU* cpu, uint16_t operand){
    return;
}

static inline void EOR(CPU* cpu, uint16_t operand){
    return;
}

static inline void LSR(CPU* cpu, uint16_t operand){
    return;
}

static inline void ADC(CPU* cpu, uint16_t operand){
    return;
}

static inline void SBC(CPU* cpu, uint16_t operand){
    return;
}

static inline void SED(CPU* cpu, uint16_t operand){
    return;
}

static inline void NOP(CPU* cpu, uint16_t operand){
    return;
}
