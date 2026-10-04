#include "Mapper071.h"

Mapper071::Mapper071(uint8_t prgBanks, uint8_t chrBanks)
    : Mapper(prgBanks, chrBanks)
{
}

Mapper::CpuTarget Mapper071::CpuMapRead(uint16_t address, uint32_t& mappedAddress)
{
    if (address < 0x8000)
        return CpuTarget::None;

    const uint32_t bank = address < 0xC000 ? selectedBank % prgBankCount : prgBankCount - 1u;
    mappedAddress = bank * kPrgBankSize + (address & 0x3FFF);
    return CpuTarget::PrgRom;
}

Mapper::CpuTarget Mapper071::CpuMapWrite(uint16_t address, uint8_t data, uint32_t& mappedAddress)
{
    (void)mappedAddress;

    if (address < 0x8000)
        return CpuTarget::None;

    if (address < 0xA000)
    {
        singleScreenControl = true;
        upperNametable = (data & 0x10) != 0;
    }
    else if (address >= 0xC000)
    {
        selectedBank = data & 0x0F;
    }

    return CpuTarget::Register;
}

bool Mapper071::PpuMapRead(uint16_t address, uint32_t& mappedAddress) const
{
    if (address > 0x1FFF)
        return false;

    mappedAddress = address;
    return true;
}

bool Mapper071::PpuMapWrite(uint16_t address, uint32_t& mappedAddress)
{
    if (address > 0x1FFF || !HasChrRam())
        return false;

    mappedAddress = address;
    return true;
}

NametableMirror Mapper071::GetMirror(NametableMirror hardwareMirror) const
{
    if (!singleScreenControl)
        return hardwareMirror;

    return upperNametable ? NametableMirror::SingleScreenHigh : NametableMirror::SingleScreenLow;
}