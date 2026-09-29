#include "Ppu2C02.h"

#include "Cartridge.h"

Ppu2C02::Ppu2C02() = default;
Ppu2C02::~Ppu2C02() = default;

void Ppu2C02::ConnectCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
}

void Ppu2C02::Clock()
{
}

uint8_t Ppu2C02::CpuRead(uint16_t address, bool readOnly)
{
    (void)readOnly;

    uint8_t data = 0x00;

    switch (address & 0x0007)
    {
        case 0x0000: break;
        case 0x0001: break;
        case 0x0002: break;
        case 0x0003: break;
        case 0x0004: break;
        case 0x0005: break;
        case 0x0006: break;
        case 0x0007: break;
        default: break;
    }

    return data;
}

void Ppu2C02::CpuWrite(uint16_t address, uint8_t data)
{
    (void)data;

    switch (address & 0x0007)
    {
        case 0x0000: break;
        case 0x0001: break;
        case 0x0002: break;
        case 0x0003: break;
        case 0x0004: break;
        case 0x0005: break;
        case 0x0006: break;
        case 0x0007: break;
        default: break;
    }
}

uint8_t Ppu2C02::PpuRead(uint16_t address, bool readOnly)
{
    (void)readOnly;

    address &= 0x3FFF;

    uint8_t data = 0x00;

    if (cartridge && cartridge->PpuRead(address, data))
        return data;

    return data;
}

void Ppu2C02::PpuWrite(uint16_t address, uint8_t data)
{
    address &= 0x3FFF;

    if (cartridge && cartridge->PpuWrite(address, data))
        return;
}
