#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Mapper;

class Cartridge
{
public:
    enum class Mirror
    {
        Horizontal,
        Vertical,
        FourScreen,
    };

    Cartridge();
    explicit Cartridge(const std::string& filename);
    ~Cartridge();

    bool CpuRead(uint16_t address, uint8_t& data) const;
    bool CpuWrite(uint16_t address, uint8_t data);

    bool PpuRead(uint16_t address, uint8_t& data) const;
    bool PpuWrite(uint16_t address, uint8_t data);

    bool ImageValid() const { return imageValid; }
    Mirror GetMirror() const { return mirror; }

private:
    void AllocateMemory(uint8_t prgBankCount, uint8_t chrBankCount);
    void CreateMapper();

    std::vector<uint8_t> prgMemory;
    std::vector<uint8_t> chrMemory;

    std::shared_ptr<Mapper> mapper;

    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;
    uint8_t mapperId = 0;

    Mirror mirror = Mirror::Horizontal;
    bool imageValid = false;
};
