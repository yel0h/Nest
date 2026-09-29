#include "Bus.h"

#include "Cartridge.h"

Bus::Bus()
{
    cpu.ConnectBus(this);
}

Bus::~Bus() = default;

void Bus::InsertCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
    ppu.ConnectCartridge(cart);
}

void Bus::Reset()
{
    cpu.Reset();
    systemClockCounter = 0;
}

void Bus::Clock()
{
    ppu.Clock();

    if (systemClockCounter % 3 == 0)
        cpu.Clock();

    ++systemClockCounter;
}

void Bus::CpuWrite(uint16_t address, uint8_t data)
{
    if (cartridge && cartridge->CpuWrite(address, data))
        return;

    if (address <= 0x1FFF)
    {
        cpuRam[address & 0x07FF] = data;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        ppu.CpuWrite(address & 0x0007, data);
    }
}

uint8_t Bus::CpuRead(uint16_t address, bool readOnly)
{
    uint8_t data = 0x00;

    if (cartridge && cartridge->CpuRead(address, data))
        return data;

    if (address <= 0x1FFF)
    {
        data = cpuRam[address & 0x07FF];
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        data = ppu.CpuRead(address & 0x0007, readOnly);
    }

    return data;
}
