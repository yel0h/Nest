#pragma once

#include <array>
#include <cstdint>

#include "Cpu6502.h"

class Bus
{
public:
    Bus();
    ~Bus();

    Cpu6502 cpu;
    std::array<uint8_t, 64 * 1024> ram{};

    void Write(uint16_t address, uint8_t data);
    uint8_t Read(uint16_t address, bool readOnly = false) const;
};
