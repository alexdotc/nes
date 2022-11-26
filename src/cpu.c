#include <stdint.h>

#include "cpu.h"
#include "mem.h"

static inline void STA(CPU*, uint16_t operand);
static inline void STX(CPU*, uint16_t operand);
static inline void STY(CPU*, uint16_t operand);
static inline void LDA(CPU*, uint16_t operand);
static inline void LDX(CPU*, uint16_t operand);
static inline void LDY(CPU*, uint16_t operand);
static inline void CPX(CPU*, uint16_t operand);
static inline void CPY(CPU*, uint16_t operand);
static inline void CMP(CPU*, uint16_t operand);
static inline void TAX(CPU*, uint16_t operand);
static inline void TAY(CPU*, uint16_t operand);
static inline void TSX(CPU*, uint16_t operand);
static inline void TXA(CPU*, uint16_t operand);
static inline void TXS(CPU*, uint16_t operand);
static inline void TYA(CPU*, uint16_t operand);
static inline void DEC(CPU*, uint16_t operand);
static inline void INC(CPU*, uint16_t operand);
static inline void INX(CPU*, uint16_t operand);
static inline void INY(CPU*, uint16_t operand);
static inline void DEX(CPU*, uint16_t operand);
static inline void BRK(CPU*, uint16_t operand);
static inline void BPL(CPU*, uint16_t operand);
static inline void BIT(CPU*, uint16_t operand);
static inline void BMI(CPU*, uint16_t operand);
static inline void BCC(CPU*, uint16_t operand);
static inline void BCS(CPU*, uint16_t operand);
static inline void BVC(CPU*, uint16_t operand);
static inline void BVS(CPU*, uint16_t operand);
static inline void BEQ(CPU*, uint16_t operand);
static inline void BNE(CPU*, uint16_t operand);
static inline void ORA(CPU*, uint16_t operand);
static inline void ASL(CPU*, uint16_t operand);
static inline void DEY(CPU*, uint16_t operand);
static inline void PLA(CPU*, uint16_t operand);
static inline void PHA(CPU*, uint16_t operand);
static inline void PHP(CPU*, uint16_t operand);
static inline void PLP(CPU*, uint16_t operand);
static inline void CLC(CPU*, uint16_t operand);
static inline void CLD(CPU*, uint16_t operand);
static inline void CLI(CPU*, uint16_t operand);
static inline void CLV(CPU*, uint16_t operand);
static inline void JMP(CPU*, uint16_t operand);
static inline void JSR(CPU*, uint16_t operand);
static inline void RTS(CPU*, uint16_t operand);
static inline void RTI(CPU*, uint16_t operand);
static inline void ROL(CPU*, uint16_t operand);
static inline void SEC(CPU*, uint16_t operand);
static inline void SEI(CPU*, uint16_t operand);
static inline void AND(CPU*, uint16_t operand);
static inline void ROR(CPU*, uint16_t operand);
static inline void EOR(CPU*, uint16_t operand);
static inline void LSR(CPU*, uint16_t operand);
static inline void ADC(CPU*, uint16_t operand);
static inline void SBC(CPU*, uint16_t operand);
static inline void SED(CPU*, uint16_t operand);
static inline void NOP(CPU*, uint16_t operand);
static inline void undef(CPU*, uint16_t operand); /* illegal opcode */

/* array of function pointers to opcode routines, indexed by opcode number */
static const void (*opcodes[256])(CPU*, uint16_t) =
{
    BRK, ORA, undef, undef, undef, ORA, ASL, undef, PHP, ORA, ASL, undef, undef, ORA, ASL, undef, /* 00-OF */
    BPL, ORA, undef, undef, undef, ORA, ASL, undef, CLC, ORA, undef, undef, undef, ORA, ASL, undef, /* 10-1F */
    JSR, AND, undef, undef, BIT, AND, ROL, undef, PLP, AND, ROL, undef, BIT, AND, ROL, undef, /* 20-2F */
    BMI, AND, undef, undef, undef, AND, ROL, undef, SEC, AND, undef, undef, undef, AND, ROL, undef, /* 30-3F */
    RTI, EOR, undef, undef, undef, EOR, LSR, undef, PHA, EOR, LSR, undef, JMP, EOR, LSR, undef, /* 40-4F */
    BVC, EOR, undef, undef, undef, EOR, LSR, undef, CLI, EOR, undef, undef, undef, EOR, LSR, undef, /* 50-5F */
    RTS, ADC, undef, undef, undef, ADC, ROR, undef, PLA, ADC, ROR, undef, JMP, ADC, ROR, undef, /* 60-6F */
    BVS, ADC, undef, undef, undef, ADC, ROR, undef, SEI, ADC, undef, undef, undef, ADC, ROR, undef, /* 70-7F */
    undef, STA, undef, undef, STY, STA, STX, undef, DEY, undef, TXA, undef, STY, STA, STX, undef, /* 80-8F */
    BCC, STA, undef, undef, STY, STA, STX, undef, TYA, STA, TXS, undef, undef, STA, undef, undef, /* 90-9F */
    LDY, LDA, LDX, undef, LDY, LDA, LDX, undef, TAY, LDA, TAX, undef, LDY, LDA, LDX, undef, /* A0-AF */
    BCS, LDA, undef, undef, LDY, LDA, LDX, undef, CLV, LDA, TSX, undef, LDY, LDA, LDX, undef, /* B0-BF */
    CPY, CMP, undef, undef, CPY, CMP, DEC, undef, INY, CMP, DEX, undef, CPY, CMP, DEC, undef, /* C0-CF */
    BNE, CMP, undef, undef, undef, CMP, DEC, undef, CLD, CMP, undef, undef, undef, CMP, DEC, undef, /* D0-DF */
    CPX, SBC, undef, undef, CPX, SBC, INC, undef, INX, SBC, NOP, undef, CPX, SBC, INC, undef, /* E0-EF */
    BEQ, SBC, undef, undef, undef, SBC, INC, undef, SED, SBC, undef, undef, undef, SBC, INC, undef /* F0-FF */
};


CPU make_cpu(Memory* mem){
    uint8_t A = 0, X = 0, Y = 0;
    uint8_t P = IRQ_DISABLE;
    uint8_t S = SP_INIT;
    uint16_t PC = 0;

    CPU cpu = { A,X,Y,S,P,PC,mem };

    return cpu;
}

void reset(CPU* cpu){
    /* set PC to address in reset vector */

    cpu->PC = (((uint16_t)(*(cpu->mem->map[RESET+1]))) << 8) + (uint16_t)(*(cpu->mem->map[RESET])); /* TODO make a 2-byte LE parser function */

}
