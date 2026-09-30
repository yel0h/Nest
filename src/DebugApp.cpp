#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "raylib.h"

#include "Bus.h"
#include "Cartridge.h"

namespace
{
    constexpr int kPixelScale = 2;
    constexpr int kScreenMarginX = 20;
    constexpr int kScreenMarginY = 20;
    constexpr int kSidePanelWidth = 260;

    constexpr int kPatternTableScale = 1;
    constexpr int kPatternTableGap = 12;
    constexpr int kPatternPanelHeight = 40 + Ppu2C02::PatternTableSize * kPatternTableScale + 30;

    constexpr int kWindowWidth =
        kScreenMarginX * 2 + Ppu2C02::ScreenWidth * kPixelScale + kSidePanelWidth;
    constexpr int kWindowHeight =
        kScreenMarginY * 2 + Ppu2C02::ScreenHeight * kPixelScale + kPatternPanelHeight;

    std::array<Color, Ppu2C02::ScreenWidth * Ppu2C02::ScreenHeight> ToRaylibPixels(const Ppu2C02::ScreenBuffer& src)
    {
        std::array<Color, Ppu2C02::ScreenWidth * Ppu2C02::ScreenHeight> out{};
        for (size_t i = 0; i < src.size(); ++i)
            out[i] = Color{src[i].r, src[i].g, src[i].b, 255};
        return out;
    }

    std::array<Color, Ppu2C02::PatternTableSize * Ppu2C02::PatternTableSize> ToRaylibPixels(
        const Ppu2C02::PatternTableBuffer& src)
    {
        std::array<Color, Ppu2C02::PatternTableSize * Ppu2C02::PatternTableSize> out{};
        for (size_t i = 0; i < src.size(); ++i)
            out[i] = Color{src[i].r, src[i].g, src[i].b, 255};
        return out;
    }

    uint8_t PollController1()
    {
        uint8_t state = 0;
        if (IsKeyDown(KEY_X)) state |= ControllerButtonA;
        if (IsKeyDown(KEY_Z)) state |= ControllerButtonB;
        if (IsKeyDown(KEY_RIGHT_SHIFT)) state |= ControllerButtonSelect;
        if (IsKeyDown(KEY_ENTER)) state |= ControllerButtonStart;
        if (IsKeyDown(KEY_UP)) state |= ControllerButtonUp;
        if (IsKeyDown(KEY_DOWN)) state |= ControllerButtonDown;
        if (IsKeyDown(KEY_LEFT)) state |= ControllerButtonLeft;
        if (IsKeyDown(KEY_RIGHT)) state |= ControllerButtonRight;
        return state;
    }

    void StepOneInstruction(Bus& nes)
    {
        do
        {
            nes.Clock();
        } while (!nes.cpu.InstructionComplete());
    }

    void StepOneFrame(Bus& nes)
    {
        do
        {
            nes.Clock();
        } while (!nes.ppu.FrameComplete());
        nes.ppu.ClearFrameComplete();
    }

    void DrawFlag(int x, int y, const char* label, bool set)
    {
        DrawText(label, x, y, 20, set ? GREEN : GRAY);
    }

    void DrawCpuPanel(Bus& nes, int x, int y)
    {
        DrawText("CPU", x, y, 22, RAYWHITE);
        y += 30;

        DrawText(TextFormat("PC: $%04X", nes.cpu.pc), x, y, 20, RAYWHITE);
        y += 24;
        DrawText(TextFormat("A: $%02X [%3d]", nes.cpu.a, nes.cpu.a), x, y, 20, RAYWHITE);
        y += 24;
        DrawText(TextFormat("X: $%02X [%3d]", nes.cpu.x, nes.cpu.x), x, y, 20, RAYWHITE);
        y += 24;
        DrawText(TextFormat("Y: $%02X [%3d]", nes.cpu.y, nes.cpu.y), x, y, 20, RAYWHITE);
        y += 24;
        DrawText(TextFormat("SP: $%02X", nes.cpu.sp), x, y, 20, RAYWHITE);
        y += 34;

        DrawText("Flags", x, y, 20, RAYWHITE);
        y += 26;
        const int flagSpacing = 30;
        DrawFlag(x + flagSpacing * 0, y, "N", nes.cpu.GetFlag(Cpu6502::Negative));
        DrawFlag(x + flagSpacing * 1, y, "V", nes.cpu.GetFlag(Cpu6502::Overflow));
        DrawFlag(x + flagSpacing * 2, y, "B", nes.cpu.GetFlag(Cpu6502::Break));
        DrawFlag(x + flagSpacing * 3, y, "D", nes.cpu.GetFlag(Cpu6502::Decimal));
        DrawFlag(x + flagSpacing * 4, y, "I", nes.cpu.GetFlag(Cpu6502::InterruptOff));
        DrawFlag(x + flagSpacing * 5, y, "Z", nes.cpu.GetFlag(Cpu6502::Zero));
        DrawFlag(x + flagSpacing * 6, y, "C", nes.cpu.GetFlag(Cpu6502::Carry));
        y += 40;

        DrawText("Memory @ PC", x, y, 20, RAYWHITE);
        y += 26;
        constexpr int bytesPerRow = 4;
        constexpr int rows = 4;
        for (int row = 0; row < rows; ++row)
        {
            char line[64] = {};
            int offset = 0;
            offset += snprintf(line + offset, sizeof(line) - offset, "$%04X:",
                                static_cast<unsigned>(nes.cpu.pc + row * bytesPerRow));
            for (int col = 0; col < bytesPerRow; ++col)
            {
                const uint16_t address = static_cast<uint16_t>(nes.cpu.pc + row * bytesPerRow + col);
                const uint8_t value = nes.CpuRead(address, true);
                offset += snprintf(line + offset, sizeof(line) - offset, " %02X", value);
            }
            DrawText(line, x, y, 18, LIGHTGRAY);
            y += 22;
        }

        y += 20;
        DrawText("[C] Step instruction", x, y, 18, GRAY);
        y += 22;
        DrawText("[F] Step frame", x, y, 18, GRAY);
        y += 22;
        DrawText("[SPACE] Run / pause", x, y, 18, GRAY);
        y += 22;
        DrawText("[R] Reset", x, y, 18, GRAY);
        y += 22;
        DrawText("[P] Cycle palette", x, y, 18, GRAY);
        y += 22;
        DrawText("[N] Toggle name tables", x, y, 18, GRAY);
        y += 22;
        DrawText("[1-4] Select name table", x, y, 18, GRAY);
        y += 22;
        DrawText("[O] Toggle OAM view", x, y, 18, GRAY);
        y += 22;
        DrawText("[I] Invert sprite/background priority", x, y, 18, GRAY);
        y += 22;
        DrawText("Pad: Arrows/Enter/RShift/X/Z", x, y, 18, GRAY);
    }

    void DrawPaletteSwatches(Ppu2C02& ppu, uint8_t selectedPalette, int x, int y)
    {
        constexpr int swatchSize = 12;
        constexpr int paletteGap = 6;

        for (int paletteId = 0; paletteId < 8; ++paletteId)
        {
            const int paletteX = x + paletteId * (swatchSize * 4 + paletteGap);

            for (int colorSlot = 0; colorSlot < 4; ++colorSlot)
            {
                const PixelColor pixel = ppu.GetColorFromPalette(static_cast<uint8_t>(paletteId),
                                                                   static_cast<uint8_t>(colorSlot));
                DrawRectangle(paletteX + colorSlot * swatchSize, y, swatchSize, swatchSize,
                              Color{pixel.r, pixel.g, pixel.b, 255});
            }

            if (paletteId == selectedPalette)
                DrawRectangleLines(paletteX - 1, y - 1, swatchSize * 4 + 2, swatchSize + 2, RAYWHITE);
        }
    }

    void DrawPatternPanel(Bus& nes, Texture2D& leftTable, Texture2D& rightTable, uint8_t selectedPalette,
                           int x, int y)
    {
        nes.ppu.RenderPatternTable(0, selectedPalette);
        nes.ppu.RenderPatternTable(1, selectedPalette);

        const auto leftPixels = ToRaylibPixels(nes.ppu.GetPatternTableView(0));
        const auto rightPixels = ToRaylibPixels(nes.ppu.GetPatternTableView(1));
        UpdateTexture(leftTable, leftPixels.data());
        UpdateTexture(rightTable, rightPixels.data());

        DrawText(TextFormat("Pattern Tables (Palette %d)", selectedPalette), x, y, 20, RAYWHITE);
        y += 26;

        const float tableSize = static_cast<float>(Ppu2C02::PatternTableSize * kPatternTableScale);
        DrawTextureEx(leftTable, Vector2{static_cast<float>(x), static_cast<float>(y)}, 0.0f,
                      static_cast<float>(kPatternTableScale), WHITE);
        DrawTextureEx(rightTable,
                      Vector2{static_cast<float>(x) + tableSize + kPatternTableGap, static_cast<float>(y)}, 0.0f,
                      static_cast<float>(kPatternTableScale), WHITE);

        y += static_cast<int>(tableSize) + 14;
        DrawPaletteSwatches(nes.ppu, selectedPalette, x, y);
    }

    const char* MirrorModeName(Cartridge::Mirror mirror)
    {
        switch (mirror)
        {
            case Cartridge::Mirror::Vertical: return "Vertical";
            case Cartridge::Mirror::FourScreen: return "Four-Screen";
            case Cartridge::Mirror::Horizontal:
            default: return "Horizontal";
        }
    }

    void DrawOamPanel(Bus& nes, int x, int y)
    {
        constexpr int entriesToShow = 26;

        DrawText("OAM Sprites (first 26) [O] back to screen", x, y, 18, RAYWHITE);
        y += 28;
        DrawText(" # Y ID AT X", x, y, 18, LIGHTGRAY);
        y += 20;

        for (int i = 0; i < entriesToShow; ++i)
        {
            const OamEntry entry = nes.ppu.GetOamEntry(static_cast<uint8_t>(i));
            const Color rowColor = entry.y < Ppu2C02::ScreenHeight ? RAYWHITE : GRAY;
            DrawText(TextFormat("%2d $%02X $%02X $%02X $%02X", i, entry.y, entry.tileId, entry.attribute, entry.x),
                      x, y, 18, rowColor);
            y += 20;
        }
    }

    void DrawNameTableIdPanel(Bus& nes, uint8_t logicalTable, int x, int y)
    {
        constexpr int tilesPerRow = 32;
        constexpr int tilesPerColumn = 30;
        constexpr int cellSize = Ppu2C02::ScreenWidth * kPixelScale / tilesPerRow;

        DrawText(TextFormat("Name Table %d of 4 (Mirror: %s) [N] back to screen [1-4] switch table",
                             logicalTable, MirrorModeName(nes.ppu.GetMirrorMode())),
                  x, y - 26, 18, RAYWHITE);

        for (int tileRow = 0; tileRow < tilesPerColumn; ++tileRow)
        {
            for (int tileColumn = 0; tileColumn < tilesPerRow; ++tileColumn)
            {
                const uint8_t id = nes.ppu.GetNameTableEntry(
                    logicalTable, static_cast<uint8_t>(tileColumn), static_cast<uint8_t>(tileRow));

                const int cellX = x + tileColumn * cellSize;
                const int cellY = y + tileRow * cellSize;

                DrawRectangleLines(cellX, cellY, cellSize, cellSize, Color{35, 35, 35, 255});
                DrawText(TextFormat("%02X", id), cellX + 1, cellY + 2, 10, id == 0 ? DARKGRAY : LIME);
            }
        }
    }
}

int main(int argc, char** argv)
{
    Bus nes;

    std::shared_ptr<Cartridge> cartridge;
    if (argc > 1)
    {
        cartridge = std::make_shared<Cartridge>(argv[1]);
        if (!cartridge->ImageValid())
            cartridge = std::make_shared<Cartridge>();
    }
    else
    {
        cartridge = std::make_shared<Cartridge>();
    }

    nes.InsertCartridge(cartridge);
    nes.Reset();

    InitWindow(kWindowWidth, kWindowHeight, "Nest - NES Debugger");
    SetTargetFPS(60);

    Image blankFrame = GenImageColor(Ppu2C02::ScreenWidth, Ppu2C02::ScreenHeight, BLACK);
    Texture2D screenTexture = LoadTextureFromImage(blankFrame);
    UnloadImage(blankFrame);

    Image blankPatternTable = GenImageColor(Ppu2C02::PatternTableSize, Ppu2C02::PatternTableSize, BLACK);
    Texture2D leftPatternTexture = LoadTextureFromImage(blankPatternTable);
    Texture2D rightPatternTexture = LoadTextureFromImage(blankPatternTable);
    UnloadImage(blankPatternTable);

    bool emulationRunning = false;
    float residualTime = 0.0f;
    uint8_t selectedPalette = 0;
    bool showNameTables = false;
    uint8_t selectedNameTable = 0;
    bool bruteForceRender = false;
    bool showOam = false;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE))
            emulationRunning = !emulationRunning;

        if (IsKeyPressed(KEY_R))
        {
            nes.Reset();
            residualTime = 0.0f;
        }

        if (IsKeyPressed(KEY_P))
            selectedPalette = (selectedPalette + 1) & 0x07;

        if (IsKeyPressed(KEY_N))
            showNameTables = !showNameTables;

        if (IsKeyPressed(KEY_O))
            showOam = !showOam;

        if (IsKeyPressed(KEY_B))
            bruteForceRender = !bruteForceRender;

        if (IsKeyPressed(KEY_I))
            nes.ppu.SetInvertSpritePriority(!nes.ppu.GetInvertSpritePriority());

        if (IsKeyPressed(KEY_ONE)) selectedNameTable = 0;
        if (IsKeyPressed(KEY_TWO)) selectedNameTable = 1;
        if (IsKeyPressed(KEY_THREE)) selectedNameTable = 2;
        if (IsKeyPressed(KEY_FOUR)) selectedNameTable = 3;

        nes.controllerState[0] = PollController1();

        if (emulationRunning)
        {
            constexpr float kFrameSeconds = 1.0f / 60.0f;
            if (residualTime > 0.0f)
            {
                residualTime -= GetFrameTime();
            }
            else
            {
                residualTime += kFrameSeconds - GetFrameTime();
                StepOneFrame(nes);
            }
        }
        else
        {
            if (IsKeyPressed(KEY_C))
                StepOneInstruction(nes);

            if (IsKeyPressed(KEY_F))
                StepOneFrame(nes);
        }

        if (bruteForceRender)
            nes.ppu.RenderFrameBruteForce();

        const auto pixels = ToRaylibPixels(nes.ppu.GetScreen());
        UpdateTexture(screenTexture, pixels.data());

        BeginDrawing();
        ClearBackground(BLACK);

        if (showNameTables)
        {
            DrawNameTableIdPanel(nes, selectedNameTable, kScreenMarginX, kScreenMarginY + 30);
        }
        else if (showOam)
        {
            DrawOamPanel(nes, kScreenMarginX, kScreenMarginY + 30);
        }
        else
        {
            DrawTextureEx(screenTexture,
                          Vector2{static_cast<float>(kScreenMarginX), static_cast<float>(kScreenMarginY)}, 0.0f,
                          static_cast<float>(kPixelScale), WHITE);

            if (bruteForceRender)
                DrawText("[B] BRUTE-FORCE RENDER", kScreenMarginX, kScreenMarginY - 20, 16, ORANGE);

            if (nes.ppu.GetInvertSpritePriority())
                DrawText("[I] PRIORITY INVERTED", kScreenMarginX, kScreenMarginY + Ppu2C02::ScreenHeight * kPixelScale + 4,
                          16, ORANGE);
        }

        DrawCpuPanel(nes, kScreenMarginX * 2 + Ppu2C02::ScreenWidth * kPixelScale, kScreenMarginY);

        DrawPatternPanel(nes, leftPatternTexture, rightPatternTexture, selectedPalette, kScreenMarginX,
                          kScreenMarginY * 2 + Ppu2C02::ScreenHeight * kPixelScale);

        EndDrawing();
    }

    UnloadTexture(leftPatternTexture);
    UnloadTexture(rightPatternTexture);
    UnloadTexture(screenTexture);
    CloseWindow();

    return 0;
}
