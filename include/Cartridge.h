#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "NametableMirror.h"

class Mapper;

class Cartridge
{
public:
    using Mirror = NametableMirror;

    Cartridge();
    explicit Cartridge(const std::string& filename);
    explicit Cartridge(std::istream& image);
    ~Cartridge();

    bool CpuRead(uint16_t address, uint8_t& data) const;
    bool CpuWrite(uint16_t address, uint8_t data);

    bool PpuRead(uint16_t address, uint8_t& data) const;
    bool PpuWrite(uint16_t address, uint8_t data);

    void NotifyPpuFetch(uint16_t address, uint64_t ppuClock);
    bool IrqPending() const;

    bool ImageValid() const { return imageValid; }
    Mirror GetMirror() const;
    uint8_t MapperId() const { return mapperId; }
    bool HasBattery() const { return battery; }

private:
    void AllocateMemory(uint8_t prgBankCount, uint8_t chrBankCount);
    bool CreateMapper();
    void LoadImage(std::istream& image);
    void LoadSaveFile();
    void WriteSaveFile() const;

    std::vector<uint8_t> prgMemory;
    std::vector<uint8_t> chrMemory;
    std::vector<uint8_t> prgRam;

    std::shared_ptr<Mapper> mapper;

    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;
    uint8_t mapperId = 0;

    Mirror mirror = Mirror::Horizontal;
    bool battery = false;
    bool prgRamDirty = false;
    std::string savePath;
    bool imageValid = false;
};
