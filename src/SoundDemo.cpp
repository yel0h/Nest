#include <algorithm>
#include <atomic>

#include "AudioSynth.h"
#include "raylib.h"

namespace
{
    constexpr int kWindowWidth = 640;
    constexpr int kWindowHeight = 360;

    constexpr double kMinFrequency = 55.0;
    constexpr double kMaxFrequency = 1760.0;
    constexpr double kMinDuty = 0.05;
    constexpr double kMaxDuty = 0.95;
    constexpr int kMaxHarmonics = 64;
    constexpr double kVolume = 0.15;

    std::atomic<bool> g_toneEnabled{false};
    std::atomic<double> g_frequency{220.0};
    std::atomic<double> g_duty{0.5};
    std::atomic<int> g_harmonics{8};

    double SoundOut(int, double globalTime, double timeStep)
    {
        if (!g_toneEnabled.load(std::memory_order_relaxed))
            return 0.0;

        return kVolume * SamplePulseWave(g_frequency.load(std::memory_order_relaxed), globalTime,
                                         g_duty.load(std::memory_order_relaxed),
                                         g_harmonics.load(std::memory_order_relaxed), timeStep);
    }

    void HandleInput()
    {
        const double dt = GetFrameTime();

        double duty = g_duty.load(std::memory_order_relaxed);
        if (IsKeyDown(KEY_Q)) duty += 0.5 * dt;
        if (IsKeyDown(KEY_A)) duty -= 0.5 * dt;
        g_duty.store(std::clamp(duty, kMinDuty, kMaxDuty), std::memory_order_relaxed);

        double frequency = g_frequency.load(std::memory_order_relaxed);
        if (IsKeyDown(KEY_E)) frequency *= 1.0 + 1.0 * dt;
        if (IsKeyDown(KEY_D)) frequency /= 1.0 + 1.0 * dt;
        g_frequency.store(std::clamp(frequency, kMinFrequency, kMaxFrequency), std::memory_order_relaxed);

        int harmonics = g_harmonics.load(std::memory_order_relaxed);
        if (IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_W)) ++harmonics;
        if (IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S)) --harmonics;
        g_harmonics.store(std::clamp(harmonics, 1, kMaxHarmonics), std::memory_order_relaxed);

        g_toneEnabled.store(IsKeyDown(KEY_SPACE), std::memory_order_relaxed);
    }

    void DrawWaveform(Rectangle area, double sampleRate)
    {
        const double frequency = g_frequency.load(std::memory_order_relaxed);
        const double duty = g_duty.load(std::memory_order_relaxed);
        const int harmonics = g_harmonics.load(std::memory_order_relaxed);

        constexpr double kPeriodsShown = 2.0;
        const double window = kPeriodsShown / frequency;
        const float midY = area.y + area.height * 0.5f;
        const float halfHeight = area.height * 0.5f * 0.8f;

        DrawRectangleLinesEx(area, 1.0f, DARKGRAY);
        DrawLine(static_cast<int>(area.x), static_cast<int>(midY),
                 static_cast<int>(area.x + area.width), static_cast<int>(midY), DARKGRAY);

        Vector2 previous{};
        const int columns = static_cast<int>(area.width);
        for (int x = 0; x < columns; ++x)
        {
            const double t = window * x / columns;
            const double value = SamplePulseWave(frequency, t, duty, harmonics, 1.0 / sampleRate);
            const Vector2 point{area.x + static_cast<float>(x),
                                midY - static_cast<float>(value) * halfHeight / 2.0f};
            if (x > 0)
                DrawLineV(previous, point, GREEN);
            previous = point;
        }
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
            HandleInput();

            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("Hold SPACE to play a pulse wave", 20, 16, 20, RAYWHITE);
            DrawText(TextFormat("freq %7.1f Hz (E up / D down)", g_frequency.load()), 20, 48, 16, GRAY);
            DrawText(TextFormat("duty %7.1f %% (Q up / A down)", g_duty.load() * 100.0), 20, 70, 16, GRAY);
            DrawText(TextFormat("harmonics %3d (W up / S down)", g_harmonics.load()), 20, 92, 16, GRAY);
            DrawText(g_toneEnabled.load() ? "playing" : "silent", 20, 118, 16,
                     g_toneEnabled.load() ? GREEN : DARKGRAY);

            DrawWaveform({20.0f, 150.0f, kWindowWidth - 40.0f, 190.0f}, synth.SampleRate());
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
