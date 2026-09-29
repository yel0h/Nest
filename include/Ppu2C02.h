#pragma once

#include <array>
#include <cstdint>
#include <memory>

class Cartridge;

struct PixelColor
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

class Ppu2C02
{
public:
    static constexpr int ScreenWidth = 256;
    static constexpr int ScreenHeight = 240;
    static constexpr int PatternTableSize = 128;

    using ScreenBuffer = std::array<PixelColor, ScreenWidth * ScreenHeight>;
    using PatternTableBuffer = std::array<PixelColor, PatternTableSize * PatternTableSize>;

    Ppu2C02();
    ~Ppu2C02();

    void ConnectCartridge(const std::shared_ptr<Cartridge>& cart);
    void Clock();

    uint8_t CpuRead(uint16_t address, bool readOnly = false);
    void CpuWrite(uint16_t address, uint8_t data);

    uint8_t PpuRead(uint16_t address, bool readOnly = false);
    void PpuWrite(uint16_t address, uint8_t data);

    const ScreenBuffer& GetScreen() const { return screen; }
    const ScreenBuffer& GetNameTableView(uint8_t index) const { return nameTableView[index & 0x01]; }
    const PatternTableBuffer& GetPatternTableView(uint8_t index) const { return patternTableView[index & 0x01]; }

    bool FrameComplete() const { return frameComplete; }
    void ClearFrameComplete() { frameComplete = false; }

private:
    void PlotPixel(int x, int y, const PixelColor& color);

    std::shared_ptr<Cartridge> cartridge;

    std::array<std::array<uint8_t, 1024>, 2> nameTable{};
    std::array<uint8_t, 32> paletteRam{};

    std::array<std::array<uint8_t, 4096>, 2> patternMemory{};

    std::array<PixelColor, 64> palette{};

    ScreenBuffer screen{};
    std::array<ScreenBuffer, 2> nameTableView{};
    std::array<PatternTableBuffer, 2> patternTableView{};

    int16_t scanline = -1;
    int16_t cycle = 0;
    bool frameComplete = false;
};
