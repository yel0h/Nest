#include "Mapper009.h"

Mapper009::Mapper009(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper009::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    const uint32_t windowCount = static_cast<uint32_t>(prgBankCount) * 2;
    const uint32_t window = address < 0xA000 ? prgBank % windowCount : windowCount - 3 + ((address - 0xA000) >> 13);
    mappedAddress = window * 0x2000u + (address & 0x1FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper009::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if (address < 0xA000)
        return CpuTarget::None;

    switch ((address >> 12) & 0x07)
    {
        case 0x2: prgBank = data & 0x0F; break;
        case 0x3: chrBank[0][0] = data & 0x1F; break;
        case 0x4: chrBank[0][1] = data & 0x1F; break;
        case 0x5: chrBank[1][0] = data & 0x1F; break;
        case 0x6: chrBank[1][1] = data & 0x1F; break;
        default:
            mirrorWritten = true;
            horizontalMirror = (data & 0x01) != 0;
            break;
    }

    return CpuTarget::Register;
}

bool Mapper009::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

bool Mapper009::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = ChrOffset(address);
    return true;
}

NametableMirror Mapper009::GetMirror(NametableMirror hardwareMirror) const
{
    if (hardwareMirror == NametableMirror::FourScreen || !mirrorWritten)
        return hardwareMirror;

    return horizontalMirror ? NametableMirror::Horizontal : NametableMirror::Vertical;
}

void Mapper009::PpuFetch(uint16_t address, uint64_t ppuClock)
{
    (void)ppuClock;

    for (int half = 0; half < 2; ++half)
    {
        if (pendingLatch[half] >= 0)
        {
            latch[half] = static_cast<uint8_t>(pendingLatch[half]);
            pendingLatch[half] = -1;
        }
    }

    if (address > 0x1FFF)
        return;

    const int half = (address >> 12) & 0x01;
    const uint16_t tileRow = address & 0x0FF8;
    if (tileRow == 0x0FD8)
        pendingLatch[half] = 0;
    else if (tileRow == 0x0FE8)
        pendingLatch[half] = 1;
}

uint32_t Mapper009::ChrOffset(uint16_t address) const
{
    const int half = (address >> 12) & 0x01;
    const uint32_t windowCount = ChrSize() / 0x1000;
    const uint32_t bank = chrBank[half][latch[half]] % windowCount;
    return bank * 0x1000u + (address & 0x0FFF);
}