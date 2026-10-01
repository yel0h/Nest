#pragma once

#include <atomic>

#include "raylib.h"

class AudioSynth
{
public:
    static constexpr int kChannelLeft = 0;
    static constexpr int kChannelRight = 1;
    static constexpr int kChannelCount = 2;

    using SampleFn = double (*)(int channel, double globalTime, double timeStep);

    explicit AudioSynth(unsigned int sampleRate = 44100, int bufferFrames = 1024);
    ~AudioSynth();

    AudioSynth(const AudioSynth&) = delete;
    AudioSynth& operator=(const AudioSynth&) = delete;

    void SetSampleFunction(SampleFn fn);

    unsigned int SampleRate() const { return m_sampleRate; }
    double GlobalTime() const { return m_globalTime.load(std::memory_order_relaxed); }

private:
    static void StreamCallback(void* buffer, unsigned int frameCount);
    void Render(short* out, unsigned int frameCount);

    static std::atomic<AudioSynth*> s_active;

    unsigned int m_sampleRate;
    AudioStream m_stream{};
    std::atomic<SampleFn> m_sampleFn{nullptr};
    std::atomic<double> m_globalTime{0.0};
};
