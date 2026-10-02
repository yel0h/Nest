#include "Apu2A03.h"

#include <algorithm>
#include <cmath>

#include "FastTrig.h"

namespace
{
    constexpr double kDutyCycles[4] = { 0.125, 0.25, 0.5, 0.75 };

    constexpr uint8_t kLengthTable[32] = {
        10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
        12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
    };

    constexpr uint16_t kNoisePeriods[16] = {
        4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
    };

    constexpr uint16_t kMinAudibleReload = 8;
    constexpr int kMaxSweepTarget = 0x7FF;

    constexpr double kCpuClockHz = 5369318.0 / 3.0;
    constexpr uint32_t kSystemClocksPerCpuClock = 3;
    constexpr double kPulseTimerPeriodsPerCycle = 16.0;
    constexpr double kTriangleTimerPeriodsPerCycle = 32.0;

    constexpr double kPi = 3.14159265358979323846;
    constexpr int kMaxPulseHarmonics = 32;
    constexpr int kMaxTriangleHarmonics = 31;
    constexpr double kHarmonicCeilingHz = 20000.0;

    constexpr double kTriangleMax = 15.0;

    constexpr double kHighPassCutoffHz = 90.0;

    constexpr uint16_t kFrameStep1 = 3729;
    constexpr uint16_t kFrameStep2 = 7457;
    constexpr uint16_t kFrameStep3 = 11186;
    constexpr uint16_t kFrameStep4 = 14915;
    constexpr uint16_t kFrameStep5 = 18641;
}

void Apu2A03::LengthCounter::Load(uint8_t tableIndex)
{
    if (enabled)
        count = kLengthTable[tableIndex & 0x1F];
}

void Apu2A03::LengthCounter::SetEnabled(bool value)
{
    enabled = value;
    if (!enabled)
        count = 0;
}

void Apu2A03::LengthCounter::Clock()
{
    if (!halt && count > 0)
        --count;
}

void Apu2A03::Envelope::Clock()
{
    if (start)
    {
        start = false;
        decay = 15;
        divider = period;
        return;
    }

    if (divider > 0)
    {
        --divider;
        return;
    }

    divider = period;
    if (decay > 0)
        --decay;
    else if (loop)
        decay = 15;
}

double Apu2A03::PulseOscillator::Sample(double phase) const
{
    if (frequency <= 0.0)
        return 0.0;

    const int harmonics = std::min(kMaxPulseHarmonics, static_cast<int>(kHarmonicCeilingHz / frequency));

    double level = dutyCycle;
    for (int n = 1; n <= harmonics; ++n)
    {
        const double weight = 2.0 * FastTrig::SinTurns(0.5 * n * dutyCycle) / (n * kPi);
        level += weight * FastTrig::CosTurns(n * (phase - dutyCycle / 2.0));
    }

    return level * amplitude;
}

void Apu2A03::PulseChannel::Reset()
{
    *this = PulseChannel(channelOne);
}

void Apu2A03::PulseChannel::WriteRegister(uint8_t index, uint8_t data)
{
    switch (index)
    {
    case 0:
        dutyMode = data >> 6;
        length.halt = (data & 0x20) != 0;
        envelope.loop = length.halt;
        envelope.constantVolume = (data & 0x10) != 0;
        envelope.period = data & 0x0F;
        break;
    case 1:
        sweepEnabled = (data & 0x80) != 0;
        sweepPeriod = (data >> 4) & 0x07;
        sweepNegate = (data & 0x08) != 0;
        sweepShift = data & 0x07;
        sweepReload = true;
        break;
    case 2:
        timerReload = (timerReload & 0x0700) | data;
        break;
    case 3:
        timerReload = static_cast<uint16_t>(((data & 0x07) << 8) | (timerReload & 0x00FF));
        length.Load(data >> 3);
        envelope.start = true;
        phase = 0.0;
        break;
    }
}

int Apu2A03::PulseChannel::SweepTarget() const
{
    const int change = timerReload >> sweepShift;
    if (!sweepNegate)
        return timerReload + change;

    return timerReload - change - (channelOne ? 1 : 0);
}

bool Apu2A03::PulseChannel::SweepSilences() const
{
    return timerReload < kMinAudibleReload || SweepTarget() > kMaxSweepTarget;
}

void Apu2A03::PulseChannel::ClockSweep()
{
    if (sweepDivider == 0 && sweepEnabled && sweepShift != 0 && !SweepSilences())
        timerReload = static_cast<uint16_t>(std::max(0, SweepTarget()));

    if (sweepDivider == 0 || sweepReload)
    {
        sweepDivider = sweepPeriod;
        sweepReload = false;
    }
    else
    {
        --sweepDivider;
    }
}

double Apu2A03::PulseChannel::Output(double elapsedSeconds)
{
    if (timerReload < kMinAudibleReload)
        return 0.0;

    const double frequency = kCpuClockHz / (kPulseTimerPeriodsPerCycle * (timerReload + 1));
    phase += frequency * elapsedSeconds;
    phase -= std::floor(phase);

    if (length.count == 0 || SweepSilences())
        return 0.0;

    PulseOscillator oscillator;
    oscillator.frequency = frequency;
    oscillator.dutyCycle = kDutyCycles[dutyMode];
    oscillator.amplitude = envelope.Volume();
    return oscillator.Sample(phase);
}

void Apu2A03::TriangleChannel::Reset()
{
    *this = TriangleChannel{};
}

void Apu2A03::TriangleChannel::WriteRegister(uint8_t index, uint8_t data)
{
    switch (index)
    {
    case 0:
        control = (data & 0x80) != 0;
        length.halt = control;
        linearReload = data & 0x7F;
        break;
    case 2:
        timerReload = (timerReload & 0x0700) | data;
        break;
    case 3:
        timerReload = static_cast<uint16_t>(((data & 0x07) << 8) | (timerReload & 0x00FF));
        length.Load(data >> 3);
        linearReloadFlag = true;
        break;
    }
}

void Apu2A03::TriangleChannel::ClockLinear()
{
    if (linearReloadFlag)
        linearCounter = linearReload;
    else if (linearCounter > 0)
        --linearCounter;

    if (!control)
        linearReloadFlag = false;
}

double Apu2A03::TriangleChannel::Output(double elapsedSeconds)
{
    const double frequency = kCpuClockHz / (kTriangleTimerPeriodsPerCycle * (timerReload + 1));

    if (length.count > 0 && linearCounter > 0)
    {
        phase += frequency * elapsedSeconds;
        phase -= std::floor(phase);
    }

    double sum = 0.0;
    for (int n = 1; n <= kMaxTriangleHarmonics && n * frequency < kHarmonicCeilingHz; n += 2)
        sum += FastTrig::CosTurns(n * phase) / (n * n);

    const double halfRange = 0.5 * kTriangleMax;
    return halfRange - halfRange * (8.0 / (kPi * kPi)) * sum;
}

void Apu2A03::NoiseChannel::Reset()
{
    *this = NoiseChannel{};
}

void Apu2A03::NoiseChannel::WriteRegister(uint8_t index, uint8_t data)
{
    switch (index)
    {
    case 0:
        length.halt = (data & 0x20) != 0;
        envelope.loop = length.halt;
        envelope.constantVolume = (data & 0x10) != 0;
        envelope.period = data & 0x0F;
        break;
    case 2:
        shortMode = (data & 0x80) != 0;
        periodClocks = kNoisePeriods[data & 0x0F] * kSystemClocksPerCpuClock;
        break;
    case 3:
        length.Load(data >> 3);
        envelope.start = true;
        break;
    }
}

void Apu2A03::NoiseChannel::ClockSystem()
{
    if (countdown > 0)
    {
        --countdown;
    }
    else
    {
        countdown = periodClocks;

        const uint16_t tap = shortMode ? (shiftRegister >> 6) : (shiftRegister >> 1);
        const uint16_t feedback = (shiftRegister ^ tap) & 0x0001;
        shiftRegister = static_cast<uint16_t>((shiftRegister >> 1) | (feedback << 14));
    }

    const bool audible = (shiftRegister & 0x0001) == 0 && length.count > 0;
    levelSum += audible ? envelope.Volume() : 0;
    ++levelSamples;
}

double Apu2A03::NoiseChannel::Output()
{
    const double level = levelSamples > 0 ? levelSum / levelSamples : 0.0;
    levelSum = 0.0;
    levelSamples = 0;
    return level;
}

void Apu2A03::Reset()
{
    pulse1.Reset();
    pulse2.Reset();
    triangle.Reset();
    noise.Reset();
    clockDivider = 0;
    frameClockCounter = 0;
    fiveStepMode = false;
    secondsSinceLastSample = 0.0;
    highPassInput = 0.0;
    highPassOutput = 0.0;
    highPassPrimed = false;
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
    else if (address >= 0x4008 && address <= 0x400B)
    {
        triangle.WriteRegister(static_cast<uint8_t>(address & 0x0003), data);
    }
    else if (address >= 0x400C && address <= 0x400F)
    {
        noise.WriteRegister(static_cast<uint8_t>(address & 0x0003), data);
    }
    else if (address == 0x4015)
    {
        pulse1.length.SetEnabled((data & 0x01) != 0);
        pulse2.length.SetEnabled((data & 0x02) != 0);
        triangle.length.SetEnabled((data & 0x04) != 0);
        noise.length.SetEnabled((data & 0x08) != 0);
    }
    else if (address == 0x4017)
    {
        fiveStepMode = (data & 0x80) != 0;
        frameClockCounter = 0;
        if (fiveStepMode)
        {
            ClockQuarterFrame();
            ClockHalfFrame();
        }
    }
}

uint8_t Apu2A03::CpuRead(uint16_t address)
{
    if (address != 0x4015)
        return 0x00;

    uint8_t status = 0x00;
    if (pulse1.length.count > 0) status |= 0x01;
    if (pulse2.length.count > 0) status |= 0x02;
    if (triangle.length.count > 0) status |= 0x04;
    if (noise.length.count > 0) status |= 0x08;
    return status;
}

void Apu2A03::ClockQuarterFrame()
{
    pulse1.envelope.Clock();
    pulse2.envelope.Clock();
    noise.envelope.Clock();
    triangle.ClockLinear();
}

void Apu2A03::ClockHalfFrame()
{
    pulse1.length.Clock();
    pulse2.length.Clock();
    triangle.length.Clock();
    noise.length.Clock();

    pulse1.ClockSweep();
    pulse2.ClockSweep();
}

void Apu2A03::Clock()
{
    secondsSinceLastSample += 1.0 / kSystemClockHz;
    noise.ClockSystem();

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
        if (!fiveStepMode)
        {
            ClockQuarterFrame();
            ClockHalfFrame();
            frameClockCounter = 0;
        }
        break;
    case kFrameStep5:
        ClockQuarterFrame();
        ClockHalfFrame();
        frameClockCounter = 0;
        break;
    }
}

double Apu2A03::GetOutputSample()
{
    const double elapsed = secondsSinceLastSample;
    secondsSinceLastSample = 0.0;

    const double pulseLevel = pulse1.Output(elapsed) + pulse2.Output(elapsed);
    const double pulseOut = pulseLevel > 0.0 ? 95.88 / (8128.0 / pulseLevel + 100.0) : 0.0;

    const double tndLevel = triangle.Output(elapsed) / 8227.0 + noise.Output() / 12241.0;
    const double tndOut = tndLevel > 0.0 ? 159.79 / (1.0 / tndLevel + 100.0) : 0.0;

    const double mixed = pulseOut + tndOut;
    if (!highPassPrimed)
    {
        highPassInput = mixed;
        highPassPrimed = true;
    }

    const double retention = std::exp(-2.0 * kPi * kHighPassCutoffHz * elapsed);
    highPassOutput = retention * (highPassOutput + mixed - highPassInput);
    highPassInput = mixed;
    return highPassOutput;
}
