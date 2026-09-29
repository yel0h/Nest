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

    if (ppu.NmiRequested())
    {
        ppu.ClearNmiRequest();
        cpu.Nmi();
    }

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
    else if (address == 0x4014)
    {
        const uint16_t page = static_cast<uint16_t>(data) << 8;
        for (uint16_t i = 0; i <= 0xFF; ++i)
        {
            ppu.WriteOamByte(static_cast<uint8_t>(i), CpuRead(static_cast<uint16_t>(page + i)));
        }
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
