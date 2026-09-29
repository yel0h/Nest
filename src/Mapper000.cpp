#include "Mapper000.h"

Mapper000::Mapper000(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

bool Mapper000::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return false;

    mappedAddress = address & (prgBankCount > 1 ? 0x7FFF : 0x3FFF);
    return true;
}

bool Mapper000::CpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return false;

    mappedAddress = address & (prgBankCount > 1 ? 0x7FFF : 0x3FFF);
    return true;
}

bool Mapper000::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = address;
    return true;
}

bool Mapper000::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || chrBankCount != 0)
        return false;

    mappedAddress = address;
    return true;
}
