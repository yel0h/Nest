#include "Mapper004.h"

namespace
{
    constexpr uint32_t kPrgWindowSize = 8 * 1024;
    constexpr uint32_t kChrWindowSize = 1024;
}

Mapper004::Mapper004(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper004::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        if (!prgRamEnabled)
            return CpuTarget::None;

        mappedAddress = address & 0x1FFF;
        return CpuTarget::PrgRam;
    }

    if (address < 0x8000)
        return CpuTarget::None;

    mappedAddress = PrgOffset(address);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper004::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        if (!prgRamEnabled || prgRamWriteProtect)
            return CpuTarget::Register;

        mappedAddress = address & 0x1FFF;
        return CpuTarget::PrgRam;
    }

    if (address < 0x8000)
        return CpuTarget::None;

    const bool oddAddress = (address & 0x0001) != 0;

    switch ((address >> 13) & 0x03)
    {
        case 0:
            if (oddAddress)
                bankRegisters[bankSelect & 0x07] = data;
            else
                bankSelect = data;
            break;

        case 1:
            if (oddAddress)
            {
                prgRamEnabled = (data & 0x80) != 0;
                prgRamWriteProtect = (data & 0x40) != 0;
            }
            else
            {
                mirrorWritten = true;
                horizontalMirror = (data & 0x01) != 0;
            }
            break;

        case 2:
            if (oddAddress)
            {
                irqCounter = 0;
                irqReload = true;
            }
            else
            {
                irqLatch = data;
            }
            break;

        default:
            irqEnabled = oddAddress;
            if (!oddAddress)
                irqPending = false;
            break;
    }

    return CpuTarget::Register;
}

bool Mapper004::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

bool Mapper004::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

NametableMirror Mapper004::GetMirror(NametableMirror hardwareMirror) const
{
    if (hardwareMirror == NametableMirror::FourScreen || !mirrorWritten)
        return hardwareMirror;

    return horizontalMirror ? NametableMirror::Horizontal : NametableMirror::Vertical;
}

void Mapper004::PpuFetch(uint16_t address, uint64_t ppuClock)
{
    const bool high = (address & 0x1000) != 0;

    if (high && !a12High && ppuClock - a12FellAt >= kMinA12LowClocks)
        ClockScanlineCounter();

    if (!high && a12High)
        a12FellAt = ppuClock;

    a12High = high;
}

void Mapper004::ClockScanlineCounter()
{
    if (irqCounter == 0 || irqReload)
    {
        irqCounter = irqLatch;
        irqReload = false;
    }
    else
    {
        --irqCounter;
    }

    if (irqCounter == 0 && irqEnabled)
        irqPending = true;
}

uint32_t Mapper004::PrgOffset(uint16_t address) const
{
    const uint32_t windowCount = static_cast<uint32_t>(prgBankCount) * 2;
    const bool swapMode = (bankSelect & 0x40) != 0;

    uint32_t window;
    switch ((address >> 13) & 0x03)
    {
        case 0:
            window = swapMode ? windowCount - 2 : (bankRegisters[6] & 0x3F);
            break;
        case 1:
            window = bankRegisters[7] & 0x3F;
            break;
        case 2:
            window = swapMode ? (bankRegisters[6] & 0x3F) : windowCount - 2;
            break;
        default:
            window = windowCount - 1;
            break;
    }

    window %= windowCount;
    return window * kPrgWindowSize + (address & 0x1FFF);
}

uint32_t Mapper004::ChrOffset(uint16_t address) const
{
    uint32_t slot = (address >> 10) & 0x07;
    if (bankSelect & 0x80)
        slot ^= 0x04;

    uint32_t window;
    if (slot < 4)
    {
        const uint8_t reg = bankRegisters[slot >> 1];
        window = (reg & 0xFE) | (slot & 0x01);
    }
    else
    {
        window = bankRegisters[2 + (slot - 4)];
    }

    const uint32_t windowCount = ChrSize() / kChrWindowSize;
    window %= windowCount;
    return window * kChrWindowSize + (address & 0x03FF);
}
