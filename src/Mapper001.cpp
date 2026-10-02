#include "Mapper001.h"

#include <algorithm>

Mapper001::Mapper001(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper001::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        mappedAddress = address & 0x1FFF;
        return CpuTarget::PrgRam;
    }

    if (address < 0x8000)
        return CpuTarget::None;

    mappedAddress = PrgOffset(address);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper001::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        mappedAddress = address & 0x1FFF;
        return CpuTarget::PrgRam;
    }

    if (address < 0x8000)
        return CpuTarget::None;

    if (data & 0x80)
    {
        shiftRegister = kShiftReset;
        controlRegister |= 0x0C;
        return CpuTarget::Register;
    }

    const bool lastWrite = (shiftRegister & 0x01) != 0;
    shiftRegister = static_cast<uint8_t>((shiftRegister >> 1) | ((data & 0x01) << 4));

    if (lastWrite)
    {
        CommitRegister(address, static_cast<uint8_t>(shiftRegister & 0x1F));
        shiftRegister = kShiftReset;
    }

    return CpuTarget::Register;
}

bool Mapper001::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

bool Mapper001::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

NametableMirror Mapper001::GetMirror(NametableMirror hardwareMirror) const
{
    (void)hardwareMirror;

    switch (controlRegister & 0x03)
    {
        case 0: return NametableMirror::SingleScreenLow;
        case 1: return NametableMirror::SingleScreenHigh;
        case 2: return NametableMirror::Vertical;
        default: return NametableMirror::Horizontal;
    }
}

void Mapper001::CommitRegister(uint16_t address, uint8_t value)
{
    switch ((address >> 13) & 0x03)
    {
        case 0: controlRegister = value; break;
        case 1: chrBank0 = value; break;
        case 2: chrBank1 = value; break;
        default: prgBank = static_cast<uint8_t>(value & 0x0F); break;
    }
}

uint32_t Mapper001::PrgOffset(uint16_t address) const
{
    const uint32_t window = std::min<uint32_t>(prgBankCount, 16);
    const uint32_t outer = (prgBankCount > 16 && (chrBank0 & 0x10)) ? 16 : 0;

    const bool upperHalf = address >= 0xC000;
    uint32_t bank;

    switch ((controlRegister >> 2) & 0x03)
    {
        case 0:
        case 1:
            bank = (prgBank & 0x0E) | (upperHalf ? 1 : 0);
            break;
        case 2:
            bank = upperHalf ? prgBank : 0;
            break;
        default:
            bank = upperHalf ? window - 1 : prgBank;
            break;
    }

    bank = outer + (bank % window);
    return (bank * kPrgBankSize + (address & 0x3FFF)) % PrgSize();
}

uint32_t Mapper001::ChrOffset(uint16_t address) const
{
    const bool upperHalf = address >= 0x1000;
    uint32_t bank;

    if (controlRegister & 0x10)
        bank = upperHalf ? chrBank1 : chrBank0;
    else
        bank = (chrBank0 & 0x1E) | (upperHalf ? 1 : 0);

    return (bank * 0x1000 + (address & 0x0FFF)) % ChrSize();
}
