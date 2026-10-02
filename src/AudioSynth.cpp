#include "AudioSynth.h"

#include <algorithm>

#include <cmath>

#include "FastTrig.h"

double SamplePulseWave(double frequency, double time, double duty, int harmonics, double timeStep)
{
    constexpr double kPi = 3.14159265358979323846;

    const double cycles = frequency * time;
    const double nyquist = 0.5 / timeStep;

    double sum = 0.0;
    for (int n = 1; n <= harmonics && n * frequency < nyquist; ++n)
    {
        const double weight = FastTrig::SinTurns(0.5 * n * duty) / n;
        sum += weight * FastTrig::CosTurns(n * (cycles - 0.5 * duty));
    }

    return (4.0 / kPi) * sum;
}

std::atomic<AudioSynth*> AudioSynth::s_active{nullptr};

AudioSynth::AudioSynth(unsigned int sampleRate, int bufferFrames)
    : m_sampleRate(sampleRate)
{
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(bufferFrames);
    m_stream = LoadAudioStream(m_sampleRate, 16, kChannelCount);

    s_active.store(this, std::memory_order_release);
    SetAudioStreamCallback(m_stream, &AudioSynth::StreamCallback);
    PlayAudioStream(m_stream);
}

AudioSynth::~AudioSynth()
{
    StopAudioStream(m_stream);
    s_active.store(nullptr, std::memory_order_release);
    UnloadAudioStream(m_stream);
    CloseAudioDevice();
}

void AudioSynth::SetSampleFunction(SampleFn fn)
{
    m_sampleFn.store(fn, std::memory_order_release);
}

void AudioSynth::StreamCallback(void* buffer, unsigned int frameCount)
{
    if (AudioSynth* synth = s_active.load(std::memory_order_acquire))
        synth->Render(static_cast<short*>(buffer), frameCount);
}

void AudioSynth::Render(short* out, unsigned int frameCount)
{
    const SampleFn fn = m_sampleFn.load(std::memory_order_acquire);
    const double step = 1.0 / static_cast<double>(m_sampleRate);
    double now = m_globalTime.load(std::memory_order_relaxed);

    for (unsigned int frame = 0; frame < frameCount; ++frame)
    {
        for (int channel = 0; channel < kChannelCount; ++channel)
        {
            const double value = fn ? fn(channel, now, step) : 0.0;
            const double clamped = std::clamp(value, -1.0, 1.0);
            out[frame * kChannelCount + channel] = static_cast<short>(clamped * 32767.0);
        }
        now += step;
    }

    m_globalTime.store(now, std::memory_order_relaxed);
}
