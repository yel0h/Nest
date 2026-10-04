#include "Mapper180.h"

Mapper180::Mapper180(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper180::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    const uint32_t bank = address < 0xC000 ? 0u : selectedBank % prgBankCount;
    mappedAddress = bank * kPrgBankSize + (address & 0x3FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper180::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if (address < 0x8000)
        return CpuTarget::None;

    selectedBank = data;
    return CpuTarget::Register;
}

bool Mapper180::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = address;
    return true;
}

bool Mapper180::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = address;
    return true;
}