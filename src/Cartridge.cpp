#include "Cartridge.h"

Cartridge::Cartridge() = default;
Cartridge::~Cartridge() = default;

bool Cartridge::CpuRead(uint16_t address, uint8_t& data) const
{
    if (address < 0x8000)
        return false;

    data = prgMemory[address - 0x8000];
    return true;
}

bool Cartridge::CpuWrite(uint16_t address, uint8_t data)
{
    if (address < 0x8000)
        return false;

    prgMemory[address - 0x8000] = data;
    return true;
}

bool Cartridge::PpuRead(uint16_t address, uint8_t& data) const
{
    if (address > 0x1FFF)
        return false;

    data = chrMemory[address];
    return true;
}

bool Cartridge::PpuWrite(uint16_t address, uint8_t data)
{
    if (address > 0x1FFF)
        return false;

    chrMemory[address] = data;
    return true;
}
