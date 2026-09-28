#include "Cpu6502.h"
#include "Bus.h"

namespace
{
    constexpr uint16_t StackBase = 0x0100;
}

void Cpu6502::Branch(bool condition)
{
    if (!condition)
        return;

    cyclesRemaining++;
    addrAbs = pc + addrRel;

    if ((addrAbs & 0xFF00) != (pc & 0xFF00))
        cyclesRemaining++;

    pc = addrAbs;
}

uint8_t Cpu6502::OP_ADC()
{
    Fetch();
    const uint16_t sum = (uint16_t)a + (uint16_t)fetched + (uint16_t)GetFlag(Carry);

    SetFlag(Carry, sum > 255);
    SetFlag(Zero, (sum & 0x00FF) == 0);
    SetFlag(Negative, sum & 0x0080);
    SetFlag(Overflow, (~((uint16_t)a ^ (uint16_t)fetched) & ((uint16_t)a ^ sum)) & 0x0080);

    a = sum & 0x00FF;
    return 1;
}

uint8_t Cpu6502::OP_SBC()
{
    Fetch();
    const uint16_t value = ((uint16_t)fetched) ^ 0x00FF;
    const uint16_t sum = (uint16_t)a + value + (uint16_t)GetFlag(Carry);

    SetFlag(Carry, sum & 0xFF00);
    SetFlag(Zero, (sum & 0x00FF) == 0);
    SetFlag(Negative, sum & 0x0080);
    SetFlag(Overflow, (sum ^ (uint16_t)a) & (sum ^ value) & 0x0080);

    a = sum & 0x00FF;
    return 1;
}

uint8_t Cpu6502::OP_AND()
{
    Fetch();
    a &= fetched;
    SetFlag(Zero, a == 0x00);
    SetFlag(Negative, a & 0x80);
    return 1;
}

uint8_t Cpu6502::OP_ASL()
{
    Fetch();
    const uint16_t result = (uint16_t)fetched << 1;
    SetFlag(Carry, (result & 0xFF00) > 0);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);

    if (instructionTable[opcode].addrMode == &Cpu6502::AM_Implied)
        a = result & 0x00FF;
    else
        Write(addrAbs, result & 0x00FF);
    return 0;
}

uint8_t Cpu6502::OP_BCC() { Branch(!GetFlag(Carry)); return 0; }
uint8_t Cpu6502::OP_BCS() { Branch(GetFlag(Carry)); return 0; }
uint8_t Cpu6502::OP_BEQ() { Branch(GetFlag(Zero)); return 0; }
uint8_t Cpu6502::OP_BMI() { Branch(GetFlag(Negative)); return 0; }
uint8_t Cpu6502::OP_BNE() { Branch(!GetFlag(Zero)); return 0; }
uint8_t Cpu6502::OP_BPL() { Branch(!GetFlag(Negative)); return 0; }
uint8_t Cpu6502::OP_BVC() { Branch(!GetFlag(Overflow)); return 0; }
uint8_t Cpu6502::OP_BVS() { Branch(GetFlag(Overflow)); return 0; }

uint8_t Cpu6502::OP_BIT()
{
    Fetch();
    const uint8_t result = a & fetched;
    SetFlag(Zero, result == 0x00);
    SetFlag(Overflow, fetched & (1 << 6));
    SetFlag(Negative, fetched & (1 << 7));
    return 0;
}

uint8_t Cpu6502::OP_BRK()
{
    pc++;

    SetFlag(InterruptOff, true);
    Write(StackBase + sp--, (pc >> 8) & 0x00FF);
    Write(StackBase + sp--, pc & 0x00FF);

    SetFlag(Break, true);
    Write(StackBase + sp--, status);
    SetFlag(Break, false);

    pc = (uint16_t)Read(0xFFFE) | ((uint16_t)Read(0xFFFF) << 8);
    return 0;
}

uint8_t Cpu6502::OP_CLC() { SetFlag(Carry, false); return 0; }
uint8_t Cpu6502::OP_CLD() { SetFlag(Decimal, false); return 0; }
uint8_t Cpu6502::OP_CLI() { SetFlag(InterruptOff, false); return 0; }
uint8_t Cpu6502::OP_CLV() { SetFlag(Overflow, false); return 0; }
uint8_t Cpu6502::OP_SEC() { SetFlag(Carry, true); return 0; }
uint8_t Cpu6502::OP_SED() { SetFlag(Decimal, true); return 0; }
uint8_t Cpu6502::OP_SEI() { SetFlag(InterruptOff, true); return 0; }

uint8_t Cpu6502::OP_CMP()
{
    Fetch();
    const uint16_t result = (uint16_t)a - (uint16_t)fetched;
    SetFlag(Carry, a >= fetched);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);
    return 1;
}

uint8_t Cpu6502::OP_CPX()
{
    Fetch();
    const uint16_t result = (uint16_t)x - (uint16_t)fetched;
    SetFlag(Carry, x >= fetched);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);
    return 0;
}

uint8_t Cpu6502::OP_CPY()
{
    Fetch();
    const uint16_t result = (uint16_t)y - (uint16_t)fetched;
    SetFlag(Carry, y >= fetched);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);
    return 0;
}

uint8_t Cpu6502::OP_DEC()
{
    Fetch();
    const uint8_t result = fetched - 1;
    Write(addrAbs, result);
    SetFlag(Zero, result == 0x00);
    SetFlag(Negative, result & 0x80);
    return 0;
}

uint8_t Cpu6502::OP_DEX() { x--; SetFlag(Zero, x == 0x00); SetFlag(Negative, x & 0x80); return 0; }
uint8_t Cpu6502::OP_DEY() { y--; SetFlag(Zero, y == 0x00); SetFlag(Negative, y & 0x80); return 0; }
uint8_t Cpu6502::OP_INX() { x++; SetFlag(Zero, x == 0x00); SetFlag(Negative, x & 0x80); return 0; }
uint8_t Cpu6502::OP_INY() { y++; SetFlag(Zero, y == 0x00); SetFlag(Negative, y & 0x80); return 0; }

uint8_t Cpu6502::OP_INC()
{
    Fetch();
    const uint8_t result = fetched + 1;
    Write(addrAbs, result);
    SetFlag(Zero, result == 0x00);
    SetFlag(Negative, result & 0x80);
    return 0;
}

uint8_t Cpu6502::OP_EOR()
{
    Fetch();
    a ^= fetched;
    SetFlag(Zero, a == 0x00);
    SetFlag(Negative, a & 0x80);
    return 1;
}

uint8_t Cpu6502::OP_ORA()
{
    Fetch();
    a |= fetched;
    SetFlag(Zero, a == 0x00);
    SetFlag(Negative, a & 0x80);
    return 1;
}

uint8_t Cpu6502::OP_JMP() { pc = addrAbs; return 0; }

uint8_t Cpu6502::OP_JSR()
{
    pc--;
    Write(StackBase + sp--, (pc >> 8) & 0x00FF);
    Write(StackBase + sp--, pc & 0x00FF);
    pc = addrAbs;
    return 0;
}

uint8_t Cpu6502::OP_RTS()
{
    sp++;
    uint16_t returnAddr = Read(StackBase + sp);
    sp++;
    returnAddr |= (uint16_t)Read(StackBase + sp) << 8;
    pc = returnAddr + 1;
    return 0;
}

uint8_t Cpu6502::OP_RTI()
{
    sp++;
    status = Read(StackBase + sp);
    status &= ~Break;
    status &= ~Unused;

    sp++;
    uint16_t returnAddr = Read(StackBase + sp);
    sp++;
    returnAddr |= (uint16_t)Read(StackBase + sp) << 8;
    pc = returnAddr;
    return 0;
}

uint8_t Cpu6502::OP_LDA() { Fetch(); a = fetched; SetFlag(Zero, a == 0x00); SetFlag(Negative, a & 0x80); return 1; }
uint8_t Cpu6502::OP_LDX() { Fetch(); x = fetched; SetFlag(Zero, x == 0x00); SetFlag(Negative, x & 0x80); return 1; }
uint8_t Cpu6502::OP_LDY() { Fetch(); y = fetched; SetFlag(Zero, y == 0x00); SetFlag(Negative, y & 0x80); return 1; }

uint8_t Cpu6502::OP_LSR()
{
    Fetch();
    SetFlag(Carry, fetched & 0x0001);
    const uint8_t result = fetched >> 1;
    SetFlag(Zero, result == 0x00);
    SetFlag(Negative, result & 0x80);

    if (instructionTable[opcode].addrMode == &Cpu6502::AM_Implied)
        a = result;
    else
        Write(addrAbs, result);
    return 0;
}

uint8_t Cpu6502::OP_NOP()
{
    return 0;
}

uint8_t Cpu6502::OP_PHA() { Write(StackBase + sp--, a); return 0; }

uint8_t Cpu6502::OP_PHP()
{
    Write(StackBase + sp--, status | Break | Unused);
    return 0;
}

uint8_t Cpu6502::OP_PLA()
{
    sp++;
    a = Read(StackBase + sp);
    SetFlag(Zero, a == 0x00);
    SetFlag(Negative, a & 0x80);
    return 0;
}

uint8_t Cpu6502::OP_PLP()
{
    sp++;
    status = Read(StackBase + sp);
    SetFlag(Unused, true);
    return 0;
}

uint8_t Cpu6502::OP_ROL()
{
    Fetch();
    const uint16_t result = ((uint16_t)fetched << 1) | GetFlag(Carry);
    SetFlag(Carry, result & 0xFF00);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);

    if (instructionTable[opcode].addrMode == &Cpu6502::AM_Implied)
        a = result & 0x00FF;
    else
        Write(addrAbs, result & 0x00FF);
    return 0;
}

uint8_t Cpu6502::OP_ROR()
{
    Fetch();
    const uint16_t result = ((uint16_t)GetFlag(Carry) << 7) | (fetched >> 1);
    SetFlag(Carry, fetched & 0x01);
    SetFlag(Zero, (result & 0x00FF) == 0);
    SetFlag(Negative, result & 0x0080);

    if (instructionTable[opcode].addrMode == &Cpu6502::AM_Implied)
        a = result & 0x00FF;
    else
        Write(addrAbs, result & 0x00FF);
    return 0;
}

uint8_t Cpu6502::OP_STA() { Write(addrAbs, a); return 0; }
uint8_t Cpu6502::OP_STX() { Write(addrAbs, x); return 0; }
uint8_t Cpu6502::OP_STY() { Write(addrAbs, y); return 0; }

uint8_t Cpu6502::OP_TAX() { x = a; SetFlag(Zero, x == 0x00); SetFlag(Negative, x & 0x80); return 0; }
uint8_t Cpu6502::OP_TAY() { y = a; SetFlag(Zero, y == 0x00); SetFlag(Negative, y & 0x80); return 0; }
uint8_t Cpu6502::OP_TSX() { x = sp; SetFlag(Zero, x == 0x00); SetFlag(Negative, x & 0x80); return 0; }
uint8_t Cpu6502::OP_TXA() { a = x; SetFlag(Zero, a == 0x00); SetFlag(Negative, a & 0x80); return 0; }
uint8_t Cpu6502::OP_TXS() { sp = x; return 0; }
uint8_t Cpu6502::OP_TYA() { a = y; SetFlag(Zero, a == 0x00); SetFlag(Negative, a & 0x80); return 0; }

uint8_t Cpu6502::OP_XXX()
{
    return 0;
}
