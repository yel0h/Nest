#include "Cartridge.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "Mapper000.h"
#include "Mapper001.h"
#include "Mapper002.h"
#include "Mapper003.h"
#include "Mapper004.h"
#include "Mapper007.h"
#include "Mapper009.h"
#include "Mapper011.h"
#include "Mapper066.h"
#include "Mapper071.h"
#include "Mapper079.h"
#include "Mapper180.h"

namespace
{
    constexpr size_t kPrgBankSize = 16 * 1024;
    constexpr size_t kChrBankSize = 8 * 1024;
    constexpr size_t kPrgRamSize = 8 * 1024;
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
    CreateMapper();
    imageValid = true;
}

Cartridge::Cartridge(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return;

    LoadImage(file);

    if (imageValid && battery)
    {
        savePath = std::filesystem::path(filename).replace_extension(".sav").string();
        LoadSaveFile();
    }
}

Cartridge::Cartridge(std::istream& image)
{
    LoadImage(image);
}

Cartridge::~Cartridge()
{
    if (battery && prgRamDirty && !savePath.empty())
        WriteSaveFile();
}

void Cartridge::LoadImage(std::istream& file)
{
    INesHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || !HasInesSignature(header) || header.prgBankCount == 0)
        return;

    if (header.flags6 & 0x04)
        file.seekg(static_cast<std::streamoff>(kTrainerSize), std::ios::cur);

    mapperId = static_cast<uint8_t>((header.flags7 & 0xF0) | (header.flags6 >> 4));
    battery = (header.flags6 & 0x02) != 0;

    if (header.flags6 & 0x08)
        mirror = Mirror::FourScreen;
    else
        mirror = (header.flags6 & 0x01) ? Mirror::Vertical : Mirror::Horizontal;

    AllocateMemory(header.prgBankCount, header.chrBankCount);
    if (!CreateMapper())
    {
        std::cerr << "Unsupported iNES mapper " << static_cast<int>(mapperId) << "\n";
        return;
    }

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

void Cartridge::LoadSaveFile()
{
    std::ifstream save(savePath, std::ios::binary);
    if (!save.is_open())
        return;

    save.read(reinterpret_cast<char*>(prgRam.data()), static_cast<std::streamsize>(prgRam.size()));
}

void Cartridge::WriteSaveFile() const
{
    std::ofstream save(savePath, std::ios::binary | std::ios::trunc);
    if (!save.is_open())
    {
        std::cerr << "Could not write save file " << savePath << "\n";
        return;
    }

    save.write(reinterpret_cast<const char*>(prgRam.data()), static_cast<std::streamsize>(prgRam.size()));
}

void Cartridge::AllocateMemory(uint8_t prgBankCount, uint8_t chrBankCount)
{
    prgBanks = prgBankCount;
    chrBanks = chrBankCount;

    prgMemory.assign(static_cast<size_t>(prgBankCount) * kPrgBankSize, 0);

    const size_t chrSize = chrBankCount > 0 ? static_cast<size_t>(chrBankCount) * kChrBankSize : kChrBankSize;
    chrMemory.assign(chrSize, 0);

    prgRam.assign(kPrgRamSize, 0);
}

bool Cartridge::CreateMapper()
{
    switch (mapperId)
    {
    case 0:
        mapper = std::make_shared<Mapper000>(prgBanks, chrBanks);
        break;
    case 1:
        mapper = std::make_shared<Mapper001>(prgBanks, chrBanks);
        break;
    case 2:
        mapper = std::make_shared<Mapper002>(prgBanks, chrBanks);
        break;
    case 3:
        mapper = std::make_shared<Mapper003>(prgBanks, chrBanks);
        break;
    case 4:
        mapper = std::make_shared<Mapper004>(prgBanks, chrBanks);
        break;
    case 7:
        mapper = std::make_shared<Mapper007>(prgBanks, chrBanks);
        break;
    case 9:
        mapper = std::make_shared<Mapper009>(prgBanks, chrBanks);
        break;
    case 11:
        mapper = std::make_shared<Mapper011>(prgBanks, chrBanks);
        break;
    case 66:
        mapper = std::make_shared<Mapper066>(prgBanks, chrBanks);
        break;
    case 71:
        mapper = std::make_shared<Mapper071>(prgBanks, chrBanks);
        break;
    case 79:
        mapper = std::make_shared<Mapper079>(prgBanks, chrBanks);
        break;
    case 180:
        mapper = std::make_shared<Mapper180>(prgBanks, chrBanks);
        break;
    default:
        mapper = nullptr;
        break;
    }

    return mapper != nullptr;
}

Cartridge::Mirror Cartridge::GetMirror() const
{
    return mapper ? mapper->GetMirror(mirror) : mirror;
}

void Cartridge::NotifyPpuFetch(uint16_t address, uint64_t ppuClock)
{
    if (mapper)
        mapper->PpuFetch(address, ppuClock);
}

bool Cartridge::IrqPending() const
{
    return mapper && mapper->IrqPending();
}

bool Cartridge::CpuRead(uint16_t address, uint8_t& data) const
{
    if (!mapper)
        return false;

    uint32_t mappedAddress = 0;
    switch (mapper->CpuMapRead(address, mappedAddress))
    {
    case Mapper::CpuTarget::PrgRom:
        data = prgMemory[mappedAddress % prgMemory.size()];
        return true;
    case Mapper::CpuTarget::PrgRam:
        data = prgRam[mappedAddress % prgRam.size()];
        return true;
    default:
        return false;
    }
}

bool Cartridge::CpuWrite(uint16_t address, uint8_t data)
{
    if (!mapper)
        return false;

    uint32_t mappedAddress = 0;
    switch (mapper->CpuMapWrite(address, data, mappedAddress))
    {
    case Mapper::CpuTarget::PrgRom:
        prgMemory[mappedAddress % prgMemory.size()] = data;
        return true;
    case Mapper::CpuTarget::PrgRam:
    {
        uint8_t& cell = prgRam[mappedAddress % prgRam.size()];
        if (cell != data)
            prgRamDirty = true;
        cell = data;
        return true;
    }
    case Mapper::CpuTarget::Register:
        return true;
    default:
        return false;
    }
}

bool Cartridge::PpuRead(uint16_t address, uint8_t& data) const
{
    if (!mapper)
        return false;

    uint32_t mappedAddress = 0;
    if (!mapper->PpuMapRead(address, mappedAddress))
        return false;

    data = chrMemory[mappedAddress % chrMemory.size()];
    return true;
}

bool Cartridge::PpuWrite(uint16_t address, uint8_t data)
{
    if (!mapper)
        return false;

    uint32_t mappedAddress = 0;
    if (!mapper->PpuMapWrite(address, mappedAddress))
        return false;

    chrMemory[mappedAddress % chrMemory.size()] = data;
    return true;
}
