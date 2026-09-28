#include "Cpu6502.h"
#include "Bus.h"

namespace
{
    constexpr uint16_t StackBase = 0x0100;
    constexpr uint16_t ResetVector = 0xFFFC;
    constexpr uint16_t IrqVector = 0xFFFE;
    constexpr uint16_t NmiVector = 0xFFFA;
}

Cpu6502::Cpu6502()
{
    using C = Cpu6502;

    instructionTable =
    {
        {"BRK",&C::OP_BRK,&C::AM_Immediate,7},{"ORA",&C::OP_ORA,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,3},{"ORA",&C::OP_ORA,&C::AM_ZeroPage,3},{"ASL",&C::OP_ASL,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"PHP",&C::OP_PHP,&C::AM_Implied,3},{"ORA",&C::OP_ORA,&C::AM_Immediate,2},{"ASL",&C::OP_ASL,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,4},{"ORA",&C::OP_ORA,&C::AM_Absolute,4},{"ASL",&C::OP_ASL,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BPL",&C::OP_BPL,&C::AM_Relative,2},{"ORA",&C::OP_ORA,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"ORA",&C::OP_ORA,&C::AM_ZeroPageX,4},{"ASL",&C::OP_ASL,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"CLC",&C::OP_CLC,&C::AM_Implied,2},{"ORA",&C::OP_ORA,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"ORA",&C::OP_ORA,&C::AM_AbsoluteX,4},{"ASL",&C::OP_ASL,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
        {"JSR",&C::OP_JSR,&C::AM_Absolute,6},{"AND",&C::OP_AND,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"BIT",&C::OP_BIT,&C::AM_ZeroPage,3},{"AND",&C::OP_AND,&C::AM_ZeroPage,3},{"ROL",&C::OP_ROL,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"PLP",&C::OP_PLP,&C::AM_Implied,4},{"AND",&C::OP_AND,&C::AM_Immediate,2},{"ROL",&C::OP_ROL,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"BIT",&C::OP_BIT,&C::AM_Absolute,4},{"AND",&C::OP_AND,&C::AM_Absolute,4},{"ROL",&C::OP_ROL,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BMI",&C::OP_BMI,&C::AM_Relative,2},{"AND",&C::OP_AND,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"AND",&C::OP_AND,&C::AM_ZeroPageX,4},{"ROL",&C::OP_ROL,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"SEC",&C::OP_SEC,&C::AM_Implied,2},{"AND",&C::OP_AND,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"AND",&C::OP_AND,&C::AM_AbsoluteX,4},{"ROL",&C::OP_ROL,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
        {"RTI",&C::OP_RTI,&C::AM_Implied,6},{"EOR",&C::OP_EOR,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,3},{"EOR",&C::OP_EOR,&C::AM_ZeroPage,3},{"LSR",&C::OP_LSR,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"PHA",&C::OP_PHA,&C::AM_Implied,3},{"EOR",&C::OP_EOR,&C::AM_Immediate,2},{"LSR",&C::OP_LSR,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"JMP",&C::OP_JMP,&C::AM_Absolute,3},{"EOR",&C::OP_EOR,&C::AM_Absolute,4},{"LSR",&C::OP_LSR,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BVC",&C::OP_BVC,&C::AM_Relative,2},{"EOR",&C::OP_EOR,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"EOR",&C::OP_EOR,&C::AM_ZeroPageX,4},{"LSR",&C::OP_LSR,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"CLI",&C::OP_CLI,&C::AM_Implied,2},{"EOR",&C::OP_EOR,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"EOR",&C::OP_EOR,&C::AM_AbsoluteX,4},{"LSR",&C::OP_LSR,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
        {"RTS",&C::OP_RTS,&C::AM_Implied,6},{"ADC",&C::OP_ADC,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,3},{"ADC",&C::OP_ADC,&C::AM_ZeroPage,3},{"ROR",&C::OP_ROR,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"PLA",&C::OP_PLA,&C::AM_Implied,4},{"ADC",&C::OP_ADC,&C::AM_Immediate,2},{"ROR",&C::OP_ROR,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"JMP",&C::OP_JMP,&C::AM_Indirect,5},{"ADC",&C::OP_ADC,&C::AM_Absolute,4},{"ROR",&C::OP_ROR,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BVS",&C::OP_BVS,&C::AM_Relative,2},{"ADC",&C::OP_ADC,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"ADC",&C::OP_ADC,&C::AM_ZeroPageX,4},{"ROR",&C::OP_ROR,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"SEI",&C::OP_SEI,&C::AM_Implied,2},{"ADC",&C::OP_ADC,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"ADC",&C::OP_ADC,&C::AM_AbsoluteX,4},{"ROR",&C::OP_ROR,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
        {"???",&C::OP_XXX,&C::AM_Implied,2},{"STA",&C::OP_STA,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,6},{"STY",&C::OP_STY,&C::AM_ZeroPage,3},{"STA",&C::OP_STA,&C::AM_ZeroPage,3},{"STX",&C::OP_STX,&C::AM_ZeroPage,3},{"???",&C::OP_XXX,&C::AM_Implied,3},{"DEY",&C::OP_DEY,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"TXA",&C::OP_TXA,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"STY",&C::OP_STY,&C::AM_Absolute,4},{"STA",&C::OP_STA,&C::AM_Absolute,4},{"STX",&C::OP_STX,&C::AM_Absolute,4},{"???",&C::OP_XXX,&C::AM_Implied,4},
        {"BCC",&C::OP_BCC,&C::AM_Relative,2},{"STA",&C::OP_STA,&C::AM_IndirectY,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,6},{"STY",&C::OP_STY,&C::AM_ZeroPageX,4},{"STA",&C::OP_STA,&C::AM_ZeroPageX,4},{"STX",&C::OP_STX,&C::AM_ZeroPageY,4},{"???",&C::OP_XXX,&C::AM_Implied,4},{"TYA",&C::OP_TYA,&C::AM_Implied,2},{"STA",&C::OP_STA,&C::AM_AbsoluteY,5},{"TXS",&C::OP_TXS,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"STA",&C::OP_STA,&C::AM_AbsoluteX,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"???",&C::OP_XXX,&C::AM_Implied,5},
        {"LDY",&C::OP_LDY,&C::AM_Immediate,2},{"LDA",&C::OP_LDA,&C::AM_IndirectX,6},{"LDX",&C::OP_LDX,&C::AM_Immediate,2},{"???",&C::OP_XXX,&C::AM_Implied,6},{"LDY",&C::OP_LDY,&C::AM_ZeroPage,3},{"LDA",&C::OP_LDA,&C::AM_ZeroPage,3},{"LDX",&C::OP_LDX,&C::AM_ZeroPage,3},{"???",&C::OP_XXX,&C::AM_Implied,3},{"TAY",&C::OP_TAY,&C::AM_Implied,2},{"LDA",&C::OP_LDA,&C::AM_Immediate,2},{"TAX",&C::OP_TAX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"LDY",&C::OP_LDY,&C::AM_Absolute,4},{"LDA",&C::OP_LDA,&C::AM_Absolute,4},{"LDX",&C::OP_LDX,&C::AM_Absolute,4},{"???",&C::OP_XXX,&C::AM_Implied,4},
        {"BCS",&C::OP_BCS,&C::AM_Relative,2},{"LDA",&C::OP_LDA,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,5},{"LDY",&C::OP_LDY,&C::AM_ZeroPageX,4},{"LDA",&C::OP_LDA,&C::AM_ZeroPageX,4},{"LDX",&C::OP_LDX,&C::AM_ZeroPageY,4},{"???",&C::OP_XXX,&C::AM_Implied,4},{"CLV",&C::OP_CLV,&C::AM_Implied,2},{"LDA",&C::OP_LDA,&C::AM_AbsoluteY,4},{"TSX",&C::OP_TSX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,4},{"LDY",&C::OP_LDY,&C::AM_AbsoluteX,4},{"LDA",&C::OP_LDA,&C::AM_AbsoluteX,4},{"LDX",&C::OP_LDX,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,4},
        {"CPY",&C::OP_CPY,&C::AM_Immediate,2},{"CMP",&C::OP_CMP,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"CPY",&C::OP_CPY,&C::AM_ZeroPage,3},{"CMP",&C::OP_CMP,&C::AM_ZeroPage,3},{"DEC",&C::OP_DEC,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"INY",&C::OP_INY,&C::AM_Implied,2},{"CMP",&C::OP_CMP,&C::AM_Immediate,2},{"DEX",&C::OP_DEX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"CPY",&C::OP_CPY,&C::AM_Absolute,4},{"CMP",&C::OP_CMP,&C::AM_Absolute,4},{"DEC",&C::OP_DEC,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BNE",&C::OP_BNE,&C::AM_Relative,2},{"CMP",&C::OP_CMP,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"CMP",&C::OP_CMP,&C::AM_ZeroPageX,4},{"DEC",&C::OP_DEC,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"CLD",&C::OP_CLD,&C::AM_Implied,2},{"CMP",&C::OP_CMP,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"CMP",&C::OP_CMP,&C::AM_AbsoluteX,4},{"DEC",&C::OP_DEC,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
        {"CPX",&C::OP_CPX,&C::AM_Immediate,2},{"SBC",&C::OP_SBC,&C::AM_IndirectX,6},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"CPX",&C::OP_CPX,&C::AM_ZeroPage,3},{"SBC",&C::OP_SBC,&C::AM_ZeroPage,3},{"INC",&C::OP_INC,&C::AM_ZeroPage,5},{"???",&C::OP_XXX,&C::AM_Implied,5},{"INX",&C::OP_INX,&C::AM_Implied,2},{"SBC",&C::OP_SBC,&C::AM_Immediate,2},{"NOP",&C::OP_NOP,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,2},{"CPX",&C::OP_CPX,&C::AM_Absolute,4},{"SBC",&C::OP_SBC,&C::AM_Absolute,4},{"INC",&C::OP_INC,&C::AM_Absolute,6},{"???",&C::OP_XXX,&C::AM_Implied,6},
        {"BEQ",&C::OP_BEQ,&C::AM_Relative,2},{"SBC",&C::OP_SBC,&C::AM_IndirectY,5},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,8},{"???",&C::OP_XXX,&C::AM_Implied,4},{"SBC",&C::OP_SBC,&C::AM_ZeroPageX,4},{"INC",&C::OP_INC,&C::AM_ZeroPageX,6},{"???",&C::OP_XXX,&C::AM_Implied,6},{"SED",&C::OP_SED,&C::AM_Implied,2},{"SBC",&C::OP_SBC,&C::AM_AbsoluteY,4},{"???",&C::OP_XXX,&C::AM_Implied,2},{"???",&C::OP_XXX,&C::AM_Implied,7},{"???",&C::OP_XXX,&C::AM_Implied,4},{"SBC",&C::OP_SBC,&C::AM_AbsoluteX,4},{"INC",&C::OP_INC,&C::AM_AbsoluteX,7},{"???",&C::OP_XXX,&C::AM_Implied,7},
    };
}

Cpu6502::~Cpu6502() = default;

bool Cpu6502::GetFlag(StatusFlag flag) const
{
    return (status & flag) != 0;
}

void Cpu6502::SetFlag(StatusFlag flag, bool value)
{
    if (value)
        status |= flag;
    else
        status &= ~flag;
}

uint8_t Cpu6502::Read(uint16_t address) const
{
    return bus->Read(address);
}

void Cpu6502::Write(uint16_t address, uint8_t data)
{
    bus->Write(address, data);
}

uint8_t Cpu6502::Fetch()
{
    if (instructionTable[opcode].addrMode != &Cpu6502::AM_Implied)
        fetched = Read(addrAbs);
    return fetched;
}

void Cpu6502::Clock()
{
    if (cyclesRemaining == 0)
    {
        opcode = Read(pc);
        pc++;

        SetFlag(Unused, true);

        const Instruction& instr = instructionTable[opcode];
        cyclesRemaining = instr.baseCycles;

        const uint8_t needsExtraFromMode = (this->*instr.addrMode)();
        const uint8_t needsExtraFromOp = (this->*instr.operate)();
        cyclesRemaining += (needsExtraFromMode & needsExtraFromOp);

        SetFlag(Unused, true);
    }

    cyclesRemaining--;
}

void Cpu6502::Reset()
{
    a = 0;
    x = 0;
    y = 0;
    sp = 0xFD;
    status = 0x00 | Unused;

    addrAbs = ResetVector;
    const uint16_t lo = Read(addrAbs);
    const uint16_t hi = Read(addrAbs + 1);
    pc = (hi << 8) | lo;

    addrRel = 0x0000;
    addrAbs = 0x0000;
    fetched = 0x00;

    cyclesRemaining = 8;
}

void Cpu6502::Irq()
{
    if (GetFlag(InterruptOff))
        return;

    Write(StackBase + sp--, (pc >> 8) & 0x00FF);
    Write(StackBase + sp--, pc & 0x00FF);

    SetFlag(Break, false);
    SetFlag(Unused, true);
    Write(StackBase + sp--, status);
    SetFlag(InterruptOff, true);

    addrAbs = IrqVector;
    const uint16_t lo = Read(addrAbs);
    const uint16_t hi = Read(addrAbs + 1);
    pc = (hi << 8) | lo;

    cyclesRemaining = 7;
}

void Cpu6502::Nmi()
{
    Write(StackBase + sp--, (pc >> 8) & 0x00FF);
    Write(StackBase + sp--, pc & 0x00FF);

    SetFlag(Break, false);
    SetFlag(Unused, true);
    Write(StackBase + sp--, status);
    SetFlag(InterruptOff, true);

    addrAbs = NmiVector;
    const uint16_t lo = Read(addrAbs);
    const uint16_t hi = Read(addrAbs + 1);
    pc = (hi << 8) | lo;

    cyclesRemaining = 8;
}
