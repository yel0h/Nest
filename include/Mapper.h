#pragma once

#include <cstdint>

class Mapper
{
public:
    Mapper(uint8_t prgBanks, uint8_t chrBanks);
    virtual ~Mapper() = default;

    virtual bool CpuMapRead(uint16_t address, uint32_t& mappedAddress) = 0;
    virtual bool CpuMapWrite(uint16_t address, uint32_t& mappedAddress) = 0;

    virtual bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const = 0;
    virtual bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) = 0;

protected:
    uint8_t prgBankCount = 0;
    uint8_t chrBankCount = 0;
};
