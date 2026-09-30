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

    uint8_t ReverseBits(uint8_t value)
    {
        value = static_cast<uint8_t>(((value & 0xF0) >> 4) | ((value & 0x0F) << 4));
        value = static_cast<uint8_t>(((value & 0xCC) >> 2) | ((value & 0x33) << 2));
        value = static_cast<uint8_t>(((value & 0xAA) >> 1) | ((value & 0x55) << 1));
        return value;
    }
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
    if (scanline == 0 && cycle == 0 && oddFrame && RenderingEnabled())
        cycle = 1;

    if (scanline >= -1 && scanline < ScreenHeight)
    {
        if (scanline == -1 && cycle == 1)
        {
            SetStatusFlag(StatusFlag::VerticalBlank, false);
            SetStatusFlag(StatusFlag::SpriteZeroHit, false);
            SetStatusFlag(StatusFlag::SpriteOverflow, false);
        }

        const bool fetchingTiles = (cycle >= 2 && cycle <= 257) || (cycle >= 321 && cycle <= 337);
        if (fetchingTiles)
        {
            UpdateBackgroundShiftRegisters();

            switch ((cycle - 1) & 0x07)
            {
                case 0:
                    LoadBackgroundShiftRegisters();
                    bgNextTileId = PpuRead(static_cast<uint16_t>(0x2000 | (vramAddress & 0x0FFF)));
                    break;

                case 2:
                {
                    const uint16_t attributeAddress = static_cast<uint16_t>(
                        0x23C0 | (vramAddress & 0x0C00) | ((vramAddress >> 4) & 0x0038) | ((vramAddress >> 2) & 0x0007));
                    bgNextTileAttribute = PpuRead(attributeAddress);
                    if (vramAddress & 0x0040)
                        bgNextTileAttribute >>= 4;
                    if (vramAddress & 0x0002)
                        bgNextTileAttribute >>= 2;
                    bgNextTileAttribute &= 0x03;
                    break;
                }

                case 4:
                {
                    const uint16_t patternTableBase = GetControlFlag(ControlFlag::BackgroundPatternTable) ? 0x1000 : 0x0000;
                    const uint16_t fineY = (vramAddress >> 12) & 0x0007;
                    bgNextTilePlaneLo = PpuRead(static_cast<uint16_t>(patternTableBase + bgNextTileId * 16 + fineY));
                    break;
                }

                case 6:
                {
                    const uint16_t patternTableBase = GetControlFlag(ControlFlag::BackgroundPatternTable) ? 0x1000 : 0x0000;
                    const uint16_t fineY = (vramAddress >> 12) & 0x0007;
                    bgNextTilePlaneHi = PpuRead(static_cast<uint16_t>(patternTableBase + bgNextTileId * 16 + fineY + 8));
                    break;
                }

                case 7:
                    IncrementCoarseX();
                    break;

                default:
                    break;
            }
        }

        if (cycle == 256)
            IncrementY();

        if (cycle == 257)
        {
            LoadBackgroundShiftRegisters();
            TransferAddressX();
        }

        if (scanline == -1 && cycle == 280)
            TransferAddressY();

        if (scanline >= 0 && cycle >= 1 && cycle <= ScreenWidth)
            RenderBackgroundPixel(cycle - 1, static_cast<int16_t>(scanline));

        if (scanline >= 0 && cycle == 257)
            EvaluateSpritesForScanline(static_cast<int16_t>(scanline));

        if (scanline >= 0 && cycle == 340)
        {
            LoadSpriteShiftRegisters(static_cast<int16_t>(scanline));
            RenderScanlineSprites(static_cast<int16_t>(scanline));
        }
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
            frameComplete = true;
            oddFrame = !oddFrame;
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
            vramAddress = static_cast<uint16_t>((vramAddress + (GetControlFlag(ControlFlag::IncrementMode) ? 32 : 1)) & 0x7FFF);
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
            vramAddressLatch = static_cast<uint16_t>((vramAddressLatch & 0xF3FF) | ((data & 0x03) << 10));
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
        case 0x0005:
            if (!addressLatch)
            {
                fineXScroll = data & 0x07;
                vramAddressLatch = static_cast<uint16_t>((vramAddressLatch & 0xFFE0) | (data >> 3));
                addressLatch = true;
            }
            else
            {
                vramAddressLatch = static_cast<uint16_t>((vramAddressLatch & 0x8C1F) |
                                                           ((data & 0x07) << 12) | ((data & 0xF8) << 2));
                addressLatch = false;
            }
            break;
        case 0x0006:
            if (!addressLatch)
            {
                vramAddressLatch = static_cast<uint16_t>(((data & 0x3F) << 8) | (vramAddressLatch & 0x00FF));
                addressLatch = true;
            }
            else
            {
                vramAddressLatch = static_cast<uint16_t>((vramAddressLatch & 0xFF00) | data);
                vramAddress = vramAddressLatch;
                addressLatch = false;
            }
            break;
        case 0x0007:
            PpuWrite(vramAddress, data);
            vramAddress = static_cast<uint16_t>((vramAddress + (GetControlFlag(ControlFlag::IncrementMode) ? 32 : 1)) & 0x7FFF);
            break;
        default: break;
    }
}

OamEntry Ppu2C02::GetOamEntry(uint8_t index) const
{
    const size_t base = static_cast<size_t>(index & 0x3F) * 4;
    return OamEntry{oam[base + 0], oam[base + 1], oam[base + 2], oam[base + 3]};
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

void Ppu2C02::RenderFrameBruteForce()
{
    RenderBackgroundLayer();
    RenderSpriteLayer();
}

bool Ppu2C02::RenderingEnabled() const
{
    return GetMaskFlag(MaskFlag::RenderBackground) || GetMaskFlag(MaskFlag::RenderSprites);
}

void Ppu2C02::IncrementY()
{
    if (!RenderingEnabled())
        return;

    if ((vramAddress & 0x7000) != 0x7000)
    {
        vramAddress = static_cast<uint16_t>(vramAddress + 0x1000);
        return;
    }

    vramAddress &= 0x8FFF;
    uint16_t coarseY = (vramAddress & 0x03E0) >> 5;
    if (coarseY == 29)
    {
        coarseY = 0;
        vramAddress ^= 0x0800;
    }
    else if (coarseY == 31)
    {
        coarseY = 0;
    }
    else
    {
        ++coarseY;
    }
    vramAddress = static_cast<uint16_t>((vramAddress & ~0x03E0) | (coarseY << 5));
}

void Ppu2C02::IncrementCoarseX()
{
    if (!RenderingEnabled())
        return;

    if ((vramAddress & 0x001F) == 31)
    {
        vramAddress &= static_cast<uint16_t>(~0x001F);
        vramAddress ^= 0x0400;
    }
    else
    {
        ++vramAddress;
    }
}

void Ppu2C02::TransferAddressX()
{
    if (!RenderingEnabled())
        return;

    vramAddress = static_cast<uint16_t>((vramAddress & ~0x041F) | (vramAddressLatch & 0x041F));
}

void Ppu2C02::TransferAddressY()
{
    if (!RenderingEnabled())
        return;

    vramAddress = static_cast<uint16_t>((vramAddress & ~0x7BE0) | (vramAddressLatch & 0x7BE0));
}

void Ppu2C02::LoadBackgroundShiftRegisters()
{
    bgPatternShiftLo = static_cast<uint16_t>((bgPatternShiftLo & 0xFF00) | bgNextTilePlaneLo);
    bgPatternShiftHi = static_cast<uint16_t>((bgPatternShiftHi & 0xFF00) | bgNextTilePlaneHi);

    bgAttributeShiftLo = static_cast<uint16_t>((bgAttributeShiftLo & 0xFF00) | ((bgNextTileAttribute & 0x01) ? 0xFF : 0x00));
    bgAttributeShiftHi = static_cast<uint16_t>((bgAttributeShiftHi & 0xFF00) | ((bgNextTileAttribute & 0x02) ? 0xFF : 0x00));
}

void Ppu2C02::UpdateBackgroundShiftRegisters()
{
    if (!GetMaskFlag(MaskFlag::RenderBackground))
        return;

    bgPatternShiftLo <<= 1;
    bgPatternShiftHi <<= 1;
    bgAttributeShiftLo <<= 1;
    bgAttributeShiftHi <<= 1;
}

void Ppu2C02::RenderBackgroundPixel(int x, int16_t y)
{
    uint8_t pixelValue = 0;
    uint8_t paletteId = 0;

    const bool showLeft = GetMaskFlag(MaskFlag::ShowBackgroundLeft);
    if (GetMaskFlag(MaskFlag::RenderBackground) && (x >= 8 || showLeft))
    {
        const uint16_t selector = static_cast<uint16_t>(0x8000 >> fineXScroll);

        const uint8_t patternLoBit = (bgPatternShiftLo & selector) ? 1 : 0;
        const uint8_t patternHiBit = (bgPatternShiftHi & selector) ? 1 : 0;
        pixelValue = static_cast<uint8_t>((patternHiBit << 1) | patternLoBit);

        const uint8_t attributeLoBit = (bgAttributeShiftLo & selector) ? 1 : 0;
        const uint8_t attributeHiBit = (bgAttributeShiftHi & selector) ? 1 : 0;
        paletteId = static_cast<uint8_t>((attributeHiBit << 1) | attributeLoBit);
    }

    const size_t index = static_cast<size_t>(y) * ScreenWidth + static_cast<size_t>(x);
    backgroundOpaque[index] = pixelValue != 0;
    PlotPixel(x, y, GetColorFromPalette(paletteId, pixelValue));
}

void Ppu2C02::EvaluateSpritesForScanline(int16_t y)
{
    scanlineSpriteCount = 0;
    scanlineHasSpriteZero = false;

    if (y < 0 || y >= ScreenHeight)
        return;

    const bool tallSprites = GetControlFlag(ControlFlag::SpriteSize);
    const int spriteHeight = tallSprites ? 16 : 8;

    int matchCount = 0;
    for (int i = 0; i < 64; ++i)
    {
        const uint8_t spriteY = oam[i * 4 + 0];
        const int row = y - (spriteY + 1);
        if (row < 0 || row >= spriteHeight)
            continue;

        ++matchCount;
        if (scanlineSpriteCount < scanlineSprites.size())
        {
            SpriteSlot& slot = scanlineSprites[scanlineSpriteCount];
            slot.y = spriteY;
            slot.tileId = oam[i * 4 + 1];
            slot.attribute = oam[i * 4 + 2];
            slot.x = oam[i * 4 + 3];
            if (i == 0)
                scanlineHasSpriteZero = true;
            ++scanlineSpriteCount;
        }
    }

    if (matchCount > 8)
        SetStatusFlag(StatusFlag::SpriteOverflow, true);
}

void Ppu2C02::LoadSpriteShiftRegisters(int16_t y)
{
    const bool tallSprites = GetControlFlag(ControlFlag::SpriteSize);
    const uint16_t spritePatternTableBase = GetControlFlag(ControlFlag::SpritePatternTable) ? 0x1000 : 0x0000;
    const int spriteHeight = tallSprites ? 16 : 8;

    for (uint8_t s = 0; s < scanlineSpriteCount; ++s)
    {
        const SpriteSlot& sprite = scanlineSprites[s];
        const bool flipVertical = (sprite.attribute & 0x80) != 0;

        int row = y - (sprite.y + 1);
        if (flipVertical)
            row = spriteHeight - 1 - row;

        uint16_t tileBase;
        if (tallSprites)
        {
            const uint16_t table = (sprite.tileId & 0x01) ? 0x1000 : 0x0000;
            uint8_t tileIndex = sprite.tileId & 0xFE;
            if (row >= 8)
            {
                tileIndex = static_cast<uint8_t>(tileIndex + 1);
                row -= 8;
            }
            tileBase = static_cast<uint16_t>(table + tileIndex * 16);
        }
        else
        {
            tileBase = static_cast<uint16_t>(spritePatternTableBase + sprite.tileId * 16);
        }

        uint8_t planeLo = PpuRead(static_cast<uint16_t>(tileBase + row));
        uint8_t planeHi = PpuRead(static_cast<uint16_t>(tileBase + row + 8));

        const bool flipHorizontal = (sprite.attribute & 0x40) != 0;
        if (flipHorizontal)
        {
            planeLo = ReverseBits(planeLo);
            planeHi = ReverseBits(planeHi);
        }

        spriteShiftPatternLo[s] = planeLo;
        spriteShiftPatternHi[s] = planeHi;
    }
}

void Ppu2C02::RenderScanlineSprites(int16_t y)
{
    if (!GetMaskFlag(MaskFlag::RenderSprites))
        return;

    const bool showLeft = GetMaskFlag(MaskFlag::ShowSpritesLeft);
    const bool bgEnabled = GetMaskFlag(MaskFlag::RenderBackground);

    std::array<bool, ScreenWidth> pixelClaimed{};

    for (uint8_t s = 0; s < scanlineSpriteCount; ++s)
    {
        const SpriteSlot& sprite = scanlineSprites[s];
        const bool behindBackground = ((sprite.attribute & 0x20) != 0) != invertSpritePriority;
        const uint8_t paletteId = static_cast<uint8_t>(4 + (sprite.attribute & 0x03));

        uint8_t shiftLo = spriteShiftPatternLo[s];
        uint8_t shiftHi = spriteShiftPatternHi[s];

        for (int col = 0; col < 8; ++col)
        {
            const int pixelX = sprite.x + col;

            const uint8_t pixelValue = static_cast<uint8_t>(((shiftHi & 0x80) >> 6) | ((shiftLo & 0x80) >> 7));
            shiftLo <<= 1;
            shiftHi <<= 1;

            if (pixelX >= ScreenWidth)
                continue;
            if (pixelX < 8 && !showLeft)
                continue;
            if (pixelValue == 0)
                continue;

            const bool bgOpaque = backgroundOpaque[static_cast<size_t>(y) * ScreenWidth + pixelX];

            if (s == 0 && scanlineHasSpriteZero && bgEnabled && bgOpaque && pixelX != 255 &&
                !GetStatusFlag(StatusFlag::SpriteZeroHit))
            {
                SetStatusFlag(StatusFlag::SpriteZeroHit, true);
            }

            if (pixelClaimed[pixelX])
                continue;
            pixelClaimed[pixelX] = true;

            if (!(behindBackground && bgOpaque))
                PlotPixel(pixelX, y, GetColorFromPalette(paletteId, pixelValue));
        }
    }
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
        const bool behindBackground = ((attribute & 0x20) != 0) != invertSpritePriority;
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

                    const int sampleRow = flipPatternTableVertical ? (tileSize - 1 - row) : row;
                    const int pixelX = tileX * tileSize + (tileSize - 1 - col);
                    const int pixelY = tileY * tileSize + sampleRow;

                    patternTableView[tableIndex][static_cast<size_t>(pixelY) * PatternTableSize + pixelX] =
                        GetColorFromPalette(paletteId, pixelValue);
                }
            }
        }
    }
}
