#pragma once

#include <array>
#include <cstdint>
#include <memory>

class Cartridge;

class Ppu2C02
{
public:
    Ppu2C02();
    ~Ppu2C02();

    void ConnectCartridge(const std::shared_ptr<Cartridge>& cart);
    void Clock();

    uint8_t CpuRead(uint16_t address, bool readOnly = false);
    void CpuWrite(uint16_t address, uint8_t data);

    uint8_t PpuRead(uint16_t address, bool readOnly = false);
    void PpuWrite(uint16_t address, uint8_t data);

private:
    std::shared_ptr<Cartridge> cartridge;

    std::array<std::array<uint8_t, 1024>, 2> nameTable{};
    std::array<uint8_t, 32> paletteRam{};

    std::array<std::array<uint8_t, 4096>, 2> patternMemory{};
};
