#include "Bus.h"

Bus::Bus()
{
    cpu.ConnectBus(this);

    for (auto& cell : ram)
        cell = 0x00;
}

Bus::~Bus() = default;

void Bus::Write(uint16_t address, uint8_t data)
{
    if (address >= 0x0000 && address <= 0xFFFF)
        ram[address] = data;
}

uint8_t Bus::Read(uint16_t address, bool readOnly) const
{
    (void)readOnly;

    if (address >= 0x0000 && address <= 0xFFFF)
        return ram[address];

    return 0x00;
}
