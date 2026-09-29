#pragma once

#include <array>
#include <cstdint>

class Cartridge
{
public:
    Cartridge();
    ~Cartridge();

    bool CpuRead(uint16_t address, uint8_t& data) const;
    bool CpuWrite(uint16_t address, uint8_t data);

    bool PpuRead(uint16_t address, uint8_t& data) const;
    bool PpuWrite(uint16_t address, uint8_t data);

private:
    std::array<uint8_t, 0x8000> prgMemory{};
    std::array<uint8_t, 0x2000> chrMemory{};
};
