#include "Mapper007.h"

Mapper007::Mapper007(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper007::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    const uint32_t bankCount32k = prgBankCount > 1 ? prgBankCount / 2u : 1u;
    mappedAddress = (selectedBank % bankCount32k) * 0x8000u + (address & 0x7FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper007::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if (address < 0x8000)
        return CpuTarget::None;

    selectedBank = data & 0x07;
    upperNametable = (data & 0x10) != 0;
    return CpuTarget::Register;
}

bool Mapper007::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = address;
    return true;
}

bool Mapper007::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = address;
    return true;
}

NametableMirror Mapper007::GetMirror(NametableMirror hardwareMirror) const
{
    (void)hardwareMirror;
    return upperNametable ? NametableMirror::SingleScreenHigh : NametableMirror::SingleScreenLow;
}
