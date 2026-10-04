#pragma once

#include "Mapper.h"

class Mapper009 : public Mapper
{
public:
    Mapper009(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper009() override = default;

    CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

    NametableMirror GetMirror(NametableMirror hardwareMirror) const override;

    void PpuFetch(uint16_t address, uint64_t ppuClock) override;
private:
    uint32_t ChrOffset(uint16_t address) const;

    uint8_t prgBank = 0x00;
    uint8_t chrBank[2][2] = {};
    uint8_t latch[2] = {1, 1};
    int8_t pendingLatch[2] = {-1, -1};
    bool horizontalMirror = false;
    bool mirrorWritten = false;};