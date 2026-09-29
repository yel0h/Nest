#include "Ppu2C02.h"

#include <random>

#include "Cartridge.h"

namespace
{
    constexpr std::array<PixelColor, 64> kMasterPalette = {{
        {84, 84, 84}, {0, 30, 116}, {8, 16, 144}, {48, 0, 136},
        {68, 0, 100}, {92, 0, 48}, {84, 4, 0}, {60, 24, 0},
        {32, 42, 0}, {8, 58, 0}, {0, 64, 0}, {0, 60, 0},
        {0, 50, 60}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
        {152, 150, 152}, {8, 76, 196}, {48, 50, 236}, {92, 30, 228},
        {136, 20, 176}, {160, 20, 100}, {152, 34, 32}, {120, 60, 0},
        {84, 90, 0}, {40, 114, 0}, {8, 124, 0}, {0, 118, 40},
        {0, 102, 120}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
        {236, 238, 236}, {76, 154, 236}, {120, 124, 236}, {176, 98, 236},
        {228, 84, 236}, {236, 88, 180}, {236, 106, 100}, {212, 136, 32},
        {160, 170, 0}, {116, 196, 0}, {76, 208, 32}, {56, 204, 108},
        {56, 180, 204}, {60, 60, 60}, {0, 0, 0}, {0, 0, 0},
        {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
        {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
        {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
        {160, 214, 228}, {160, 162, 160}, {0, 0, 0}, {0, 0, 0},
    }};

    constexpr int kCyclesPerScanline = 341;
    constexpr int kScanlinesPerFrame = 261;

    constexpr uint8_t kNoiseDarkIndex = 0x0F;
    constexpr uint8_t kNoiseLightIndex = 0x30;
}

Ppu2C02::Ppu2C02()
    : palette(kMasterPalette)
{
}

Ppu2C02::~Ppu2C02() = default;

void Ppu2C02::ConnectCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
}

void Ppu2C02::Clock()
{
    static std::mt19937 noiseGenerator{std::random_device{}()};
    static std::uniform_int_distribution<int> coinFlip(0, 1);

    if (cycle >= 1 && cycle <= ScreenWidth && scanline >= 0 && scanline < ScreenHeight)
    {
        const uint8_t colorIndex = coinFlip(noiseGenerator) ? kNoiseLightIndex : kNoiseDarkIndex;
        PlotPixel(cycle - 1, scanline, palette[colorIndex]);
    }

    ++cycle;
    if (cycle >= kCyclesPerScanline)
    {
        cycle = 0;
        ++scanline;
        if (scanline >= kScanlinesPerFrame)
        {
            scanline = -1;
            frameComplete = true;
        }
    }
}

void Ppu2C02::PlotPixel(int x, int y, const PixelColor& color)
{
    if (x < 0 || x >= ScreenWidth || y < 0 || y >= ScreenHeight)
        return;

    screen[static_cast<size_t>(y) * ScreenWidth + static_cast<size_t>(x)] = color;
}

uint8_t Ppu2C02::CpuRead(uint16_t address, bool readOnly)
{
    (void)readOnly;

    uint8_t data = 0x00;

    switch (address & 0x0007)
    {
        case 0x0000: break;
        case 0x0001: break;
        case 0x0002: break;
        case 0x0003: break;
        case 0x0004: break;
        case 0x0005: break;
        case 0x0006: break;
        case 0x0007: break;
        default: break;
    }

    return data;
}

void Ppu2C02::CpuWrite(uint16_t address, uint8_t data)
{
    (void)data;

    switch (address & 0x0007)
    {
        case 0x0000: break;
        case 0x0001: break;
        case 0x0002: break;
        case 0x0003: break;
        case 0x0004: break;
        case 0x0005: break;
        case 0x0006: break;
        case 0x0007: break;
        default: break;
    }
}

uint8_t Ppu2C02::PpuRead(uint16_t address, bool readOnly)
{
    (void)readOnly;

    address &= 0x3FFF;

    uint8_t data = 0x00;

    if (cartridge && cartridge->PpuRead(address, data))
        return data;

    return data;
}

void Ppu2C02::PpuWrite(uint16_t address, uint8_t data)
{
    address &= 0x3FFF;

    if (cartridge && cartridge->PpuWrite(address, data))
        return;
}
