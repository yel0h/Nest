#pragma once

#include <cstdint>

#include "NametableMirror.h"

class Mapper
{
public:
    enum class CpuTarget
    {
        None,
        PrgRom,
        PrgRam,
        Register,
    };

    Mapper(uint8_t prgBanks, uint8_t chrBanks);
    virtual ~Mapper() = default;

    virtual CpuTarget CpuMapRead(uint16_t address, uint32_t& mappedAddress) = 0;
    virtual CpuTarget CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress) = 0;

    virtual bool PpuMapRead(uint16_t address, uint32_t& mappedAddress) const = 0;
    virtual bool PpuMapWrite(uint16_t address, uint32_t& mappedAddress) = 0;

    virtual NametableMirror GetMirror(NametableMirror hardwareMirror) const { return hardwareMirror; }

    virtual void PpuFetch(uint16_t address, uint64_t ppuClock)
    {
        (void)address;
        (void)ppuClock;
    }
    virtual bool IrqPending() const { return false; }

protected:
    static constexpr uint32_t kPrgBankSize = 16 * 1024;
    static constexpr uint32_t kChrBankSize = 8 * 1024;

    uint32_t PrgSize() const { return static_cast<uint32_t>(prgBankCount) * kPrgBankSize; }
    uint32_t ChrSize() const { return (chrBankCount > 0 ? chrBankCount : 1u) * kChrBankSize; }
    bool HasChrRam() const { return chrBankCount == 0; }

    uint8_t prgBankCount = 0;
    uint8_t chrBankCount = 0;
};
