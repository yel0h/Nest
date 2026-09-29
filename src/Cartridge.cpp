#include "Cartridge.h"

#include <fstream>

namespace
{
    constexpr size_t kPrgBankSize = 16 * 1024;
    constexpr size_t kChrBankSize = 8 * 1024;
    constexpr size_t kTrainerSize = 512;

#pragma pack(push, 1)
    struct INesHeader
    {
        char signature[4];
        uint8_t prgBankCount;
        uint8_t chrBankCount;
        uint8_t flags6;
        uint8_t flags7;
        uint8_t prgRamSize;
        uint8_t flags9;
        uint8_t flags10;
        uint8_t reserved[5];
    };
#pragma pack(pop)

    bool HasInesSignature(const INesHeader& header)
    {
        static const char signature[4] = {'N', 'E', 'S', 0x1A};
        for (int i = 0; i < 4; ++i)
        {
            if (header.signature[i] != signature[i])
                return false;
        }
        return true;
    }
}

Cartridge::Cartridge()
{
    AllocateMemory(2, 1);
    imageValid = true;
}

Cartridge::Cartridge(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return;

    INesHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || !HasInesSignature(header))
        return;

    if (header.flags6 & 0x04)
        file.seekg(static_cast<std::streamoff>(kTrainerSize), std::ios::cur);

    mapperId = static_cast<uint8_t>((header.flags7 & 0xF0) | (header.flags6 >> 4));

    if (header.flags6 & 0x08)
        mirror = Mirror::FourScreen;
    else
        mirror = (header.flags6 & 0x01) ? Mirror::Vertical : Mirror::Horizontal;

    AllocateMemory(header.prgBankCount, header.chrBankCount);

    file.read(reinterpret_cast<char*>(prgMemory.data()), static_cast<std::streamsize>(prgMemory.size()));
    if (!file)
        return;

    if (chrBanks > 0)
    {
        file.read(reinterpret_cast<char*>(chrMemory.data()), static_cast<std::streamsize>(chrMemory.size()));
        if (!file)
            return;
    }

    imageValid = true;
}

Cartridge::~Cartridge() = default;

void Cartridge::AllocateMemory(uint8_t prgBankCount, uint8_t chrBankCount)
{
    prgBanks = prgBankCount;
    chrBanks = chrBankCount;

    prgMemory.assign(static_cast<size_t>(prgBankCount) * kPrgBankSize, 0);

    const size_t chrSize = chrBankCount > 0 ? static_cast<size_t>(chrBankCount) * kChrBankSize : kChrBankSize;
    chrMemory.assign(chrSize, 0);
}

bool Cartridge::CpuRead(uint16_t address, uint8_t& data) const
{
    if (address < 0x8000 || prgMemory.empty())
        return false;

    const uint16_t mask = prgBanks > 1 ? 0x7FFF : 0x3FFF;
    data = prgMemory[address & mask];
    return true;
}

bool Cartridge::CpuWrite(uint16_t address, uint8_t data)
{
    if (address < 0x8000 || prgMemory.empty())
        return false;

    const uint16_t mask = prgBanks > 1 ? 0x7FFF : 0x3FFF;
    prgMemory[address & mask] = data;
    return true;
}

bool Cartridge::PpuRead(uint16_t address, uint8_t& data) const
{
    if (address > 0x1FFF || chrMemory.empty())
        return false;

    data = chrMemory[address];
    return true;
}

bool Cartridge::PpuWrite(uint16_t address, uint8_t data)
{
    if (address > 0x1FFF || chrMemory.empty())
        return false;

    chrMemory[address] = data;
    return true;
}
