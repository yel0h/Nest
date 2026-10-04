#pragma once

#include "Mapper.h"

class Mapper071 : public Mapper
{
public:
    Mapper071(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper071() override = default;

    CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

    NametableMirror GetMirror(NametableMirror hardwareMirror) const override;
private:
    uint8_t selectedBank = 0x00;
    bool singleScreenControl = false;
    bool upperNametable = false;};