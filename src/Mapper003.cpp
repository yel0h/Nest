#include "Mapper003.h"

Mapper003::Mapper003(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper003::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    mappedAddress = address & (prgBankCount > 1 ? 0x7FFF : 0x3FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper003::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if (address < 0x8000)
        return CpuTarget::None;

    selectedChrBank = data;
    return CpuTarget::Register;
}

bool Mapper003::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = (selectedChrBank % (chrBankCount > 0 ? chrBankCount : 1)) * kChrBankSize + address;
    return true;
}

bool Mapper003::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = address;
    return true;
}
