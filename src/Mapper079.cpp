#include "Mapper079.h"

Mapper079::Mapper079(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper079::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    const uint32_t bankCount32k = prgBankCount > 1 ? prgBankCount / 2u : 1u;
    mappedAddress = (selectedPrg % bankCount32k) * 0x8000u + (address & 0x7FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper079::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if ((address & 0xE100) != 0x4100)
        return CpuTarget::None;

    selectedPrg = (data >> 3) & 0x01;
    selectedChr = data & 0x07;
    return CpuTarget::Register;
}

bool Mapper079::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = (selectedChr % (chrBankCount > 0 ? chrBankCount : 1)) * kChrBankSize + address;
    return true;
}

bool Mapper079::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = address;
    return true;
}