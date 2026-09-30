#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "Cartridge.h"

struct PixelColor
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct OamEntry
{
    uint8_t y = 0;
    uint8_t tileId = 0;
    uint8_t attribute = 0;
    uint8_t x = 0;
};

class Ppu2C02
{
public:
    static constexpr int ScreenWidth = 256;
    static constexpr int ScreenHeight = 240;
    static constexpr int PatternTableSize = 128;

    enum ControlFlag : uint8_t
    {
        NametableX = 1 << 0,
        NametableY = 1 << 1,
        IncrementMode = 1 << 2,
        SpritePatternTable = 1 << 3,
        BackgroundPatternTable = 1 << 4,
        SpriteSize = 1 << 5,
        SlaveMode = 1 << 6,
        EnableNmi = 1 << 7,
    };

    enum MaskFlag : uint8_t
    {
        Grayscale = 1 << 0,
        ShowBackgroundLeft = 1 << 1,
        ShowSpritesLeft = 1 << 2,
        RenderBackground = 1 << 3,
        RenderSprites = 1 << 4,
        EmphasizeRed = 1 << 5,
        EmphasizeGreen = 1 << 6,
        EmphasizeBlue = 1 << 7,
    };

    enum StatusFlag : uint8_t
    {
        SpriteOverflow = 1 << 5,
        SpriteZeroHit = 1 << 6,
        VerticalBlank = 1 << 7,
    };

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

    void WriteOamByte(uint8_t address, uint8_t data) { oam[address] = data; }
    OamEntry GetOamEntry(uint8_t index) const;
    static constexpr int OamEntryCount = 64;

    const ScreenBuffer& GetScreen() const { return screen; }
    const PatternTableBuffer& GetPatternTableView(uint8_t index) const { return patternTableView[index & 0x01]; }

    void RenderPatternTable(uint8_t tableIndex, uint8_t paletteId);

    void RenderFrameBruteForce();

    uint8_t GetNameTableEntry(uint8_t logicalTable, uint8_t tileColumn, uint8_t tileRow);
    Cartridge::Mirror GetMirrorMode() const;

    PixelColor GetColorFromPalette(uint8_t paletteId, uint8_t pixelValue);

    bool FrameComplete() const { return frameComplete; }
    void ClearFrameComplete() { frameComplete = false; }

    bool GetControlFlag(ControlFlag flag) const;
    void SetControlFlag(ControlFlag flag, bool value);

    bool GetMaskFlag(MaskFlag flag) const;
    void SetMaskFlag(MaskFlag flag, bool value);

    bool GetStatusFlag(StatusFlag flag) const;
    void SetStatusFlag(StatusFlag flag, bool value);

    bool NmiRequested() const { return nmiRequested; }
    void ClearNmiRequest() { nmiRequested = false; }

private:
    void PlotPixel(int x, int y, const PixelColor& color);

    void RenderBackgroundLayer();
    void RenderSpriteLayer();
    uint8_t GetBackgroundPaletteId(uint8_t logicalTable, uint8_t tileColumn, uint8_t tileRow);

    bool RenderingEnabled() const;

    void IncrementY();
    void IncrementCoarseX();
    void TransferAddressX();
    void TransferAddressY();

    void LoadBackgroundShiftRegisters();
    void UpdateBackgroundShiftRegisters();
    void RenderBackgroundPixel(int x, int16_t y);

    void EvaluateSpritesForScanline(int16_t y);
    void RenderScanlineSprites(int16_t y);

    std::shared_ptr<Cartridge> cartridge;

    std::array<std::array<uint8_t, 1024>, 2> nameTable{};
    std::array<uint8_t, 32> paletteRam{};

    std::array<std::array<uint8_t, 4096>, 2> patternMemory{};

    std::array<uint8_t, 256> oam{};
    uint8_t oamAddress = 0x00;

    std::array<PixelColor, 64> palette{};

    uint8_t controlRegister = 0x00;
    uint8_t maskRegister = 0x00;
    uint8_t statusRegister = 0x00;

    bool addressLatch = false;
    uint8_t dataBuffer = 0x00;
    uint16_t vramAddress = 0x0000;
    uint16_t vramAddressLatch = 0x0000;
    uint8_t fineXScroll = 0x00;

    uint8_t bgNextTileId = 0x00;
    uint8_t bgNextTileAttribute = 0x00;
    uint8_t bgNextTilePlaneLo = 0x00;
    uint8_t bgNextTilePlaneHi = 0x00;

    uint16_t bgPatternShiftLo = 0x0000;
    uint16_t bgPatternShiftHi = 0x0000;
    uint16_t bgAttributeShiftLo = 0x0000;
    uint16_t bgAttributeShiftHi = 0x0000;

    struct SpriteSlot
    {
        uint8_t y = 0xFF;
        uint8_t tileId = 0xFF;
        uint8_t attribute = 0xFF;
        uint8_t x = 0xFF;
    };
    std::array<SpriteSlot, 8> scanlineSprites{};
    uint8_t scanlineSpriteCount = 0;
    bool scanlineHasSpriteZero = false;

    ScreenBuffer screen{};
    std::array<bool, ScreenWidth * ScreenHeight> backgroundOpaque{};
    std::array<PatternTableBuffer, 2> patternTableView{};

    int16_t scanline = -1;
    int16_t cycle = 0;
    bool frameComplete = false;
    bool nmiRequested = false;
};
