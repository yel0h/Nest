#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "Cpu6502.h"
#include "Ppu2C02.h"

class Cartridge;

class Bus
{
public:
    Bus();
    ~Bus();

    Cpu6502 cpu;
    Ppu2C02 ppu;
    std::array<uint8_t, 2 * 1024> cpuRam{};

    void InsertCartridge(const std::shared_ptr<Cartridge>& cart);
    void Reset();
    void Clock();

    void CpuWrite(uint16_t address, uint8_t data);
    uint8_t CpuRead(uint16_t address, bool readOnly = false);

private:
    std::shared_ptr<Cartridge> cartridge;
    uint32_t systemClockCounter = 0;
};
