#pragma once

#include <array>

#include "Mapper.h"

class Mapper004 : public Mapper
{
public:
    Mapper004(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper004() override = default;

    CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
    CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) override;

    bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const override;
    bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) override;

    NametableMirror GetMirror(NametableMirror hardwareMirror) const override;

    void PpuFetch(uint16_t address, uint64_t ppuClock) override;
    bool IrqPending() const override { return irqPending; }

private:
    void ClockScanlineCounter();
    uint32_t PrgOffset(uint16_t address) const;
    uint32_t ChrOffset(uint16_t address) const;

    static constexpr uint64_t kMinA12LowClocks = 9;

    std::array<uint8_t, 8> bankRegisters{};
    uint8_t bankSelect = 0x00;

    bool mirrorWritten = false;
    bool horizontalMirror = false;

    bool prgRamEnabled = true;
    bool prgRamWriteProtect = false;

    uint8_t irqLatch = 0x00;
    uint8_t irqCounter = 0x00;
    bool irqReload = false;
    bool irqEnabled = false;
    bool irqPending = false;

    bool a12High = false;
    uint64_t a12FellAt = 0;
};
