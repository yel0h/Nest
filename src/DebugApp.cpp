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

    constexpr int kWindowWidth =
        kScreenMarginX * 2 + Ppu2C02::ScreenWidth * kPixelScale + kSidePanelWidth;
    constexpr int kWindowHeight = kScreenMarginY * 2 + Ppu2C02::ScreenHeight * kPixelScale;

    std::array<Color, Ppu2C02::ScreenWidth * Ppu2C02::ScreenHeight> ToRaylibPixels(const Ppu2C02::ScreenBuffer& src)
    {
        std::array<Color, Ppu2C02::ScreenWidth * Ppu2C02::ScreenHeight> out{};
        for (size_t i = 0; i < src.size(); ++i)
            out[i] = Color{src[i].r, src[i].g, src[i].b, 255};
        return out;
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

    bool emulationRunning = false;
    float residualTime = 0.0f;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE))
            emulationRunning = !emulationRunning;

        if (IsKeyPressed(KEY_R))
        {
            nes.Reset();
            residualTime = 0.0f;
        }

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

        const auto pixels = ToRaylibPixels(nes.ppu.GetScreen());
        UpdateTexture(screenTexture, pixels.data());

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTextureEx(screenTexture, Vector2{static_cast<float>(kScreenMarginX), static_cast<float>(kScreenMarginY)},
                      0.0f, static_cast<float>(kPixelScale), WHITE);

        DrawCpuPanel(nes, kScreenMarginX * 2 + Ppu2C02::ScreenWidth * kPixelScale, kScreenMarginY);

        EndDrawing();
    }

    UnloadTexture(screenTexture);
    CloseWindow();

    return 0;
}
