#include "Cpu6502.h"
#include "Bus.h"

uint8_t Cpu6502::AM_Implied()
{
    fetched = a;
    return 0;
}

uint8_t Cpu6502::AM_Immediate()
{
    addrAbs = pc++;
    return 0;
}

uint8_t Cpu6502::AM_ZeroPage()
{
    addrAbs = Read(pc++) & 0x00FF;
    return 0;
}

uint8_t Cpu6502::AM_ZeroPageX()
{
    addrAbs = (Read(pc++) + x) & 0x00FF;
    return 0;
}

uint8_t Cpu6502::AM_ZeroPageY()
{
    addrAbs = (Read(pc++) + y) & 0x00FF;
    return 0;
}

uint8_t Cpu6502::AM_Relative()
{
    addrRel = Read(pc++);
    if (addrRel & 0x80)
        addrRel |= 0xFF00;
    return 0;
}

uint8_t Cpu6502::AM_Absolute()
{
    const uint16_t lo = Read(pc++);
    const uint16_t hi = Read(pc++);
    addrAbs = (hi << 8) | lo;
    return 0;
}

uint8_t Cpu6502::AM_AbsoluteX()
{
    const uint16_t lo = Read(pc++);
    const uint16_t hi = Read(pc++);
    addrAbs = ((hi << 8) | lo) + x;
    return ((addrAbs & 0xFF00) != (hi << 8)) ? 1 : 0;
}

uint8_t Cpu6502::AM_AbsoluteY()
{
    const uint16_t lo = Read(pc++);
    const uint16_t hi = Read(pc++);
    addrAbs = ((hi << 8) | lo) + y;
    return ((addrAbs & 0xFF00) != (hi << 8)) ? 1 : 0;
}

uint8_t Cpu6502::AM_Indirect()
{
    const uint16_t ptrLo = Read(pc++);
    const uint16_t ptrHi = Read(pc++);
    const uint16_t ptr = (ptrHi << 8) | ptrLo;

    uint16_t lo, hi;
    if (ptrLo == 0x00FF)
    {
        lo = Read(ptr);
        hi = Read(ptr & 0xFF00);
    }
    else
    {
        lo = Read(ptr);
        hi = Read(ptr + 1);
    }

    addrAbs = (hi << 8) | lo;
    return 0;
}

uint8_t Cpu6502::AM_IndirectX()
{
    const uint16_t base = Read(pc++);
    const uint16_t lo = Read((base + x) & 0x00FF);
    const uint16_t hi = Read((base + x + 1) & 0x00FF);
    addrAbs = (hi << 8) | lo;
    return 0;
}

uint8_t Cpu6502::AM_IndirectY()
{
    const uint16_t base = Read(pc++);
    const uint16_t lo = Read(base & 0x00FF);
    const uint16_t hi = Read((base + 1) & 0x00FF);
    addrAbs = ((hi << 8) | lo) + y;
    return ((addrAbs & 0xFF00) != (hi << 8)) ? 1 : 0;
}
