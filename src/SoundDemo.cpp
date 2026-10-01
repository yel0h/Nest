#include <atomic>
#include <cmath>

#include "AudioSynth.h"
#include "raylib.h"

namespace
{
    constexpr int kWindowWidth = 480;
    constexpr int kWindowHeight = 200;

    constexpr double kTwoPi = 6.283185307179586;
    constexpr double kToneHz = 440.0;
    constexpr double kToneVolume = 0.25;

    std::atomic<bool> g_toneEnabled{false};

    double SoundOut(int, double globalTime, double)
    {
        if (!g_toneEnabled.load(std::memory_order_relaxed))
            return 0.0;

        return kToneVolume * std::sin(kTwoPi * kToneHz * globalTime);
    }
}

int main()
{
    InitWindow(kWindowWidth, kWindowHeight, "Sound Demo");
    SetTargetFPS(60);

    {
        AudioSynth synth;
        synth.SetSampleFunction(SoundOut);

        while (!WindowShouldClose())
        {
            g_toneEnabled.store(IsKeyDown(KEY_SPACE), std::memory_order_relaxed);

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Hold SPACE to play a 440 Hz tone", 20, 20, 20, RAYWHITE);
            DrawText(TextFormat("%u Hz, stereo, 16-bit", synth.SampleRate()), 20, 56, 20, GRAY);
            DrawText(IsKeyDown(KEY_SPACE) ? "playing" : "silent", 20, 92, 20,
                     IsKeyDown(KEY_SPACE) ? GREEN : DARKGRAY);
            DrawText(TextFormat("t = %.2f s", synth.GlobalTime()), 20, 128, 20, GRAY);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
