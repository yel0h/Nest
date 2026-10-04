#pragma once

#include "Mapper.h"

class Mapper180 : public Mapper
{
public:
    Mapper180(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper180() override = default;

    CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

private:
    uint8_t selectedBank = 0x00;};