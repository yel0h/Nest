#pragma once

#include "Mapper.h"

class Mapper000 : public Mapper
{
public:
    Mapper000(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper000() override = default;

    bool CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    bool CpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;
};
