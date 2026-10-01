#include <atomic>
#include <cmath>
#include <cstdint>

#include "raylib.h"

namespace
{
    constexpr int kWindowWidth = 480;
    constexpr int kWindowHeight = 200;

    constexpr unsigned int kSampleRate = 44100;
    constexpr unsigned int kBitsPerSample = 16;
    constexpr unsigned int kChannels = 1;
    constexpr int kStreamBufferFrames = 1024;

    constexpr double kTwoPi = 6.283185307179586;
    constexpr double kToneHz = 440.0;
    constexpr double kToneVolume = 0.25;

    std::atomic<bool> g_toneEnabled{false};

    void FillAudioBuffer(void* buffer, unsigned int frameCount)
    {
        static double phase = 0.0;
        const double phaseStep = kTwoPi * kToneHz / static_cast<double>(kSampleRate);
        const bool enabled = g_toneEnabled.load(std::memory_order_relaxed);

        int16_t* out = static_cast<int16_t*>(buffer);
        for (unsigned int i = 0; i < frameCount; ++i)
        {
            const double sample = enabled ? std::sin(phase) * kToneVolume : 0.0;
            out[i] = static_cast<int16_t>(sample * INT16_MAX);

            phase += phaseStep;
            if (phase >= kTwoPi)
                phase -= kTwoPi;
        }
    }

    class AudioOutput
    {
    public:
        AudioOutput()
        {
            InitAudioDevice();
            SetAudioStreamBufferSizeDefault(kStreamBufferFrames);
            m_stream = LoadAudioStream(kSampleRate, kBitsPerSample, kChannels);
            SetAudioStreamCallback(m_stream, FillAudioBuffer);
            PlayAudioStream(m_stream);
        }

        ~AudioOutput()
        {
            StopAudioStream(m_stream);
            UnloadAudioStream(m_stream);
            CloseAudioDevice();
        }

        AudioOutput(const AudioOutput&) = delete;
        AudioOutput& operator=(const AudioOutput&) = delete;

    private:
        AudioStream m_stream{};
    };
}

int main()
{
    InitWindow(kWindowWidth, kWindowHeight, "Sound Demo");
    SetTargetFPS(60);

    {
        AudioOutput audio;

        while (!WindowShouldClose())
        {
            g_toneEnabled.store(IsKeyDown(KEY_SPACE), std::memory_order_relaxed);

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Hold SPACE to play a 440 Hz tone", 20, 20, 20, RAYWHITE);
            DrawText("44100 Hz, mono, 16-bit", 20, 56, 20, GRAY);
            DrawText(IsKeyDown(KEY_SPACE) ? "playing" : "silent", 20, 92, 20,
                     IsKeyDown(KEY_SPACE) ? GREEN : DARKGRAY);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
