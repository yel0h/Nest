#include "Apu2A03.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr double kDutyCycles[4] = { 0.125, 0.25, 0.5, 0.75 };

    constexpr uint16_t kMinAudibleReload = 8;

    constexpr double kCpuClockHz = 5369318.0 / 3.0;
    constexpr double kTimerPeriodsPerCycle = 16.0;

    constexpr double kPi = 3.14159265358979323846;
    constexpr int kMaxHarmonics = 32;
    constexpr double kHarmonicCeilingHz = 20000.0;

    constexpr uint16_t kFrameStep1 = 3729;
    constexpr uint16_t kFrameStep2 = 7457;
    constexpr uint16_t kFrameStep3 = 11186;
    constexpr uint16_t kFrameStep4 = 14915;
}

double Apu2A03::PulseOscillator::Sample(double time) const
{
    if (frequency <= 0.0)
        return 0.0;

    double phase = time * frequency;
    phase -= std::floor(phase);

    const int harmonics = std::min(kMaxHarmonics, static_cast<int>(kHarmonicCeilingHz / frequency));

    double level = dutyCycle;
    for (int n = 1; n <= harmonics; ++n)
    {
        const double weight = 2.0 * std::sin(n * kPi * dutyCycle) / (n * kPi);
        level += weight * std::cos(2.0 * kPi * n * (phase - dutyCycle / 2.0));
    }

    return level * amplitude;
}

void Apu2A03::PulseChannel::Reset()
{
    *this = PulseChannel{};
}

void Apu2A03::PulseChannel::WriteRegister(uint8_t index, uint8_t data)
{
    switch (index)
    {
    case 0:
        dutyMode = data >> 6;
        volume = data & 0x0F;
        break;
    case 1:
        break;
    case 2:
        timerReload = (timerReload & 0x0700) | data;
        break;
    case 3:
        timerReload = static_cast<uint16_t>(((data & 0x07) << 8) | (timerReload & 0x00FF));
        break;
    }
}

double Apu2A03::PulseChannel::Output(double time) const
{
    if (!enabled || timerReload < kMinAudibleReload)
        return 0.0;

    PulseOscillator oscillator;
    oscillator.frequency = kCpuClockHz / (kTimerPeriodsPerCycle * (timerReload + 1));
    oscillator.dutyCycle = kDutyCycles[dutyMode];
    oscillator.amplitude = volume;
    return oscillator.Sample(time);
}

void Apu2A03::Reset()
{
    pulse1.Reset();
    pulse2.Reset();
    clockDivider = 0;
    frameClockCounter = 0;
    globalTime = 0.0;
}

void Apu2A03::CpuWrite(uint16_t address, uint8_t data)
{
    if (address >= 0x4000 && address <= 0x4003)
    {
        pulse1.WriteRegister(static_cast<uint8_t>(address & 0x0003), data);
    }
    else if (address >= 0x4004 && address <= 0x4007)
    {
        pulse2.WriteRegister(static_cast<uint8_t>(address & 0x0003), data);
    }
    else if (address == 0x4015)
    {
        pulse1.enabled = (data & 0x01) != 0;
        pulse2.enabled = (data & 0x02) != 0;
    }
}

void Apu2A03::ClockQuarterFrame()
{
}

void Apu2A03::ClockHalfFrame()
{
}

void Apu2A03::Clock()
{
    globalTime += 1.0 / kSystemClockHz;

    if (++clockDivider < kSystemClocksPerApuClock)
        return;

    clockDivider = 0;

    ++frameClockCounter;
    switch (frameClockCounter)
    {
    case kFrameStep1:
        ClockQuarterFrame();
        break;
    case kFrameStep2:
        ClockQuarterFrame();
        ClockHalfFrame();
        break;
    case kFrameStep3:
        ClockQuarterFrame();
        break;
    case kFrameStep4:
        ClockQuarterFrame();
        ClockHalfFrame();
        frameClockCounter = 0;
        break;
    }
}

double Apu2A03::GetOutputSample() const
{
    const double pulseLevel = pulse1.Output(globalTime) + pulse2.Output(globalTime);
    if (pulseLevel == 0.0)
        return 0.0;

    return 95.88 / (8128.0 / pulseLevel + 100.0);
}
