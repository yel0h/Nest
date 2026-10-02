#pragma once

#include "Mapper.h"

class Mapper001 : public Mapper
{
public:
    Mapper001(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper001() override = default;

    CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

    NametableMirror GetMirror(NametableMirror hardwareMirror) const override;

private:
    void CommitRegister(uint16_t address, uint8_t value);
    uint32_t PrgOffset(uint16_t address) const;
    uint32_t ChrOffset(uint16_t address) const;

    static constexpr uint8_t kShiftReset = 0x10;

    uint8_t shiftRegister = kShiftReset;
    uint8_t controlRegister = 0x0C;
    uint8_t chrBank0 = 0x00;
    uint8_t chrBank1 = 0x00;
    uint8_t prgBank = 0x00;
};
