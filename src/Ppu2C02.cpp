#include "Ppu2C02.h"

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
    if (scanline == -1 && cycle == 1)
    {
        SetStatusFlag(StatusFlag::VerticalBlank, false);
    }
    else if (scanline == ScreenHeight + 1 && cycle == 1)
    {
        SetStatusFlag(StatusFlag::VerticalBlank, true);
        if (GetControlFlag(ControlFlag::EnableNmi))
            nmiRequested = true;
    }

    ++cycle;
    if (cycle >= kCyclesPerScanline)
    {
        cycle = 0;
        ++scanline;
        if (scanline >= kScanlinesPerFrame)
        {
            scanline = -1;
            RenderFrame();
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
        case 0x0002:
            data = (statusRegister & 0xE0) | (dataBuffer & 0x1F);
            SetStatusFlag(StatusFlag::VerticalBlank, false);
            addressLatch = false;
            break;
        case 0x0003: break;
        case 0x0004:
            data = oam[oamAddress];
            break;
        case 0x0005: break;
        case 0x0006: break;
        case 0x0007:
            if (vramAddress >= 0x3F00)
            {
                data = PpuRead(vramAddress);
            }
            else
            {
                data = dataBuffer;
                dataBuffer = PpuRead(vramAddress);
            }
            vramAddress += GetControlFlag(ControlFlag::IncrementMode) ? 32 : 1;
            break;
        default: break;
    }

    return data;
}

void Ppu2C02::CpuWrite(uint16_t address, uint8_t data)
{
    switch (address & 0x0007)
    {
        case 0x0000:
            controlRegister = data;
            break;
        case 0x0001:
            maskRegister = data;
            break;
        case 0x0002: break;
        case 0x0003:
            oamAddress = data;
            break;
        case 0x0004:
            oam[oamAddress] = data;
            ++oamAddress;
            break;
        case 0x0005: break;
        case 0x0006:
            if (!addressLatch)
            {
                vramAddress = static_cast<uint16_t>((data << 8) | (vramAddress & 0x00FF));
                addressLatch = true;
            }
            else
            {
                vramAddress = static_cast<uint16_t>((vramAddress & 0xFF00) | data);
                addressLatch = false;
            }
            break;
        case 0x0007:
            PpuWrite(vramAddress, data);
            vramAddress += GetControlFlag(ControlFlag::IncrementMode) ? 32 : 1;
            break;
        default: break;
    }
}

bool Ppu2C02::GetControlFlag(ControlFlag flag) const
{
    return (controlRegister & flag) != 0;
}

void Ppu2C02::SetControlFlag(ControlFlag flag, bool value)
{
    if (value)
        controlRegister |= flag;
    else
        controlRegister &= static_cast<uint8_t>(~flag);
}

bool Ppu2C02::GetMaskFlag(MaskFlag flag) const
{
    return (maskRegister & flag) != 0;
}

void Ppu2C02::SetMaskFlag(MaskFlag flag, bool value)
{
    if (value)
        maskRegister |= flag;
    else
        maskRegister &= static_cast<uint8_t>(~flag);
}

bool Ppu2C02::GetStatusFlag(StatusFlag flag) const
{
    return (statusRegister & flag) != 0;
}

void Ppu2C02::SetStatusFlag(StatusFlag flag, bool value)
{
    if (value)
        statusRegister |= flag;
    else
        statusRegister &= static_cast<uint8_t>(~flag);
}

namespace
{
    uint8_t ResolvePaletteRamIndex(uint16_t address)
    {
        uint16_t index = address & 0x001F;
        if (index == 0x10 || index == 0x14 || index == 0x18 || index == 0x1C)
            index &= 0x000F;
        return static_cast<uint8_t>(index);
    }

    uint8_t ResolveNameTableIndex(uint16_t offset, Cartridge::Mirror mirror)
    {
        const uint8_t quadrant = static_cast<uint8_t>(offset >> 10);
        if (mirror == Cartridge::Mirror::Vertical)
            return quadrant & 0x01;
        return quadrant >> 1;
    }
}

uint8_t Ppu2C02::PpuRead(uint16_t address, bool readOnly)
{
    (void)readOnly;

    address &= 0x3FFF;

    uint8_t data = 0x00;

    if (cartridge && cartridge->PpuRead(address, data))
        return data;

    if (address <= 0x1FFF)
    {
        data = patternMemory[(address >> 12) & 0x01][address & 0x0FFF];
    }
    else if (address <= 0x3EFF)
    {
        const uint16_t offset = address & 0x0FFF;
        const uint8_t table = ResolveNameTableIndex(offset, cartridge ? cartridge->GetMirror() : Cartridge::Mirror::Horizontal);
        data = nameTable[table][offset & 0x03FF];
    }
    else
    {
        data = paletteRam[ResolvePaletteRamIndex(address)];
    }

    return data;
}

void Ppu2C02::PpuWrite(uint16_t address, uint8_t data)
{
    address &= 0x3FFF;

    if (cartridge && cartridge->PpuWrite(address, data))
        return;

    if (address <= 0x1FFF)
    {
        patternMemory[(address >> 12) & 0x01][address & 0x0FFF] = data;
    }
    else if (address <= 0x3EFF)
    {
        const uint16_t offset = address & 0x0FFF;
        const uint8_t table = ResolveNameTableIndex(offset, cartridge ? cartridge->GetMirror() : Cartridge::Mirror::Horizontal);
        nameTable[table][offset & 0x03FF] = data;
    }
    else
    {
        paletteRam[ResolvePaletteRamIndex(address)] = data;
    }
}

uint8_t Ppu2C02::GetNameTableEntry(uint8_t logicalTable, uint8_t tileColumn, uint8_t tileRow)
{
    logicalTable &= 0x03;
    tileColumn &= 0x1F;
    tileRow %= 30;

    const uint16_t address = static_cast<uint16_t>(0x2000 + logicalTable * 0x0400 + tileRow * 32 + tileColumn);
    return PpuRead(address, true);
}

Cartridge::Mirror Ppu2C02::GetMirrorMode() const
{
    return cartridge ? cartridge->GetMirror() : Cartridge::Mirror::Horizontal;
}

PixelColor Ppu2C02::GetColorFromPalette(uint8_t paletteId, uint8_t pixelValue)
{
    const uint16_t entryAddress = static_cast<uint16_t>(0x3F00 + (paletteId << 2) + pixelValue);
    return palette[PpuRead(entryAddress) & 0x3F];
}

void Ppu2C02::RenderFrame()
{
    RenderBackgroundLayer();
    RenderSpriteLayer();
}

uint8_t Ppu2C02::GetBackgroundPaletteId(uint8_t logicalTable, uint8_t tileColumn, uint8_t tileRow)
{
    logicalTable &= 0x03;

    const uint16_t attributeAddress = static_cast<uint16_t>(
        0x2000 + logicalTable * 0x0400 + 0x03C0 + (tileRow / 4) * 8 + (tileColumn / 4));
    const uint8_t attribute = PpuRead(attributeAddress, true);

    uint8_t shift = 0;
    if ((tileColumn % 4) >= 2)
        shift += 2;
    if ((tileRow % 4) >= 2)
        shift += 4;

    return (attribute >> shift) & 0x03;
}

void Ppu2C02::RenderBackgroundLayer()
{
    const PixelColor backdrop = GetColorFromPalette(0, 0);
    backgroundOpaque.fill(false);

    if (!GetMaskFlag(MaskFlag::RenderBackground))
    {
        screen.fill(backdrop);
        return;
    }

    const uint8_t baseNameTable = static_cast<uint8_t>(
        (GetControlFlag(ControlFlag::NametableX) ? 1 : 0) | (GetControlFlag(ControlFlag::NametableY) ? 2 : 0));
    const uint16_t patternTableBase = GetControlFlag(ControlFlag::BackgroundPatternTable) ? 0x1000 : 0x0000;

    constexpr int tilesPerRow = 32;
    constexpr int tilesPerColumn = 30;
    constexpr int tileSize = 8;

    for (int tileRow = 0; tileRow < tilesPerColumn; ++tileRow)
    {
        for (int tileColumn = 0; tileColumn < tilesPerRow; ++tileColumn)
        {
            const uint8_t tileId = GetNameTableEntry(baseNameTable, static_cast<uint8_t>(tileColumn),
                                                       static_cast<uint8_t>(tileRow));
            const uint8_t paletteId = GetBackgroundPaletteId(baseNameTable, static_cast<uint8_t>(tileColumn),
                                                                static_cast<uint8_t>(tileRow));
            const uint16_t tileBase = static_cast<uint16_t>(patternTableBase + tileId * 16);

            for (int row = 0; row < tileSize; ++row)
            {
                const uint8_t planeLo = PpuRead(static_cast<uint16_t>(tileBase + row));
                const uint8_t planeHi = PpuRead(static_cast<uint16_t>(tileBase + row + tileSize));

                for (int col = 0; col < tileSize; ++col)
                {
                    const uint8_t bit = static_cast<uint8_t>(7 - col);
                    const uint8_t pixelValue =
                        static_cast<uint8_t>(((planeHi >> bit) & 0x01) << 1 | ((planeLo >> bit) & 0x01));

                    const int pixelX = tileColumn * tileSize + col;
                    const int pixelY = tileRow * tileSize + row;

                    if (pixelValue == 0)
                    {
                        PlotPixel(pixelX, pixelY, backdrop);
                        continue;
                    }

                    backgroundOpaque[static_cast<size_t>(pixelY) * ScreenWidth + pixelX] = true;
                    PlotPixel(pixelX, pixelY, GetColorFromPalette(paletteId, pixelValue));
                }
            }
        }
    }
}

void Ppu2C02::RenderSpriteLayer()
{
    if (!GetMaskFlag(MaskFlag::RenderSprites))
        return;

    const bool tallSprites = GetControlFlag(ControlFlag::SpriteSize);
    const uint16_t spritePatternTableBase = GetControlFlag(ControlFlag::SpritePatternTable) ? 0x1000 : 0x0000;
    const int spriteHeight = tallSprites ? 16 : 8;

    for (int i = 63; i >= 0; --i)
    {
        const uint8_t spriteY = oam[i * 4 + 0];
        const uint8_t tileIndex = oam[i * 4 + 1];
        const uint8_t attribute = oam[i * 4 + 2];
        const uint8_t spriteX = oam[i * 4 + 3];

        const bool flipHorizontal = (attribute & 0x40) != 0;
        const bool flipVertical = (attribute & 0x80) != 0;
        const bool behindBackground = (attribute & 0x20) != 0;
        const uint8_t paletteId = static_cast<uint8_t>(4 + (attribute & 0x03));

        uint16_t tileBase;
        if (tallSprites)
        {
            const uint16_t table = (tileIndex & 0x01) ? 0x1000 : 0x0000;
            tileBase = static_cast<uint16_t>(table + (tileIndex & 0xFE) * 16);
        }
        else
        {
            tileBase = static_cast<uint16_t>(spritePatternTableBase + tileIndex * 16);
        }

        for (int row = 0; row < spriteHeight; ++row)
        {
            int sampleRow = flipVertical ? (spriteHeight - 1 - row) : row;
            uint16_t rowTileBase = tileBase;
            if (tallSprites && sampleRow >= 8)
            {
                rowTileBase += 16;
                sampleRow -= 8;
            }

            const uint8_t planeLo = PpuRead(static_cast<uint16_t>(rowTileBase + sampleRow));
            const uint8_t planeHi = PpuRead(static_cast<uint16_t>(rowTileBase + sampleRow + 8));

            for (int col = 0; col < 8; ++col)
            {
                const uint8_t bit = static_cast<uint8_t>(flipHorizontal ? col : (7 - col));
                const uint8_t pixelValue =
                    static_cast<uint8_t>(((planeHi >> bit) & 0x01) << 1 | ((planeLo >> bit) & 0x01));

                if (pixelValue == 0)
                    continue;

                const int pixelX = spriteX + col;
                const int pixelY = spriteY + 1 + row;

                if (pixelX < 0 || pixelX >= ScreenWidth || pixelY < 0 || pixelY >= ScreenHeight)
                    continue;

                if (behindBackground && backgroundOpaque[static_cast<size_t>(pixelY) * ScreenWidth + pixelX])
                    continue;

                PlotPixel(pixelX, pixelY, GetColorFromPalette(paletteId, pixelValue));
            }
        }
    }
}

void Ppu2C02::RenderPatternTable(uint8_t tableIndex, uint8_t paletteId)
{
    tableIndex &= 0x01;
    paletteId &= 0x07;

    constexpr int tilesPerSide = 16;
    constexpr int tileSize = 8;
    constexpr int bytesPerTile = 16;

    for (int tileY = 0; tileY < tilesPerSide; ++tileY)
    {
        for (int tileX = 0; tileX < tilesPerSide; ++tileX)
        {
            const uint16_t tileOffset = static_cast<uint16_t>((tileY * tilesPerSide + tileX) * bytesPerTile);
            const uint16_t tileBase = static_cast<uint16_t>(tableIndex * 0x1000 + tileOffset);

            for (int row = 0; row < tileSize; ++row)
            {
                uint8_t planeLo = PpuRead(static_cast<uint16_t>(tileBase + row));
                uint8_t planeHi = PpuRead(static_cast<uint16_t>(tileBase + row + tileSize));

                for (int col = 0; col < tileSize; ++col)
                {
                    const uint8_t pixelValue = static_cast<uint8_t>((planeLo & 0x01) | ((planeHi & 0x01) << 1));
                    planeLo >>= 1;
                    planeHi >>= 1;

                    const int pixelX = tileX * tileSize + (tileSize - 1 - col);
                    const int pixelY = tileY * tileSize + row;

                    patternTableView[tableIndex][static_cast<size_t>(pixelY) * PatternTableSize + pixelX] =
                        GetColorFromPalette(paletteId, pixelValue);
                }
            }
        }
    }
}
