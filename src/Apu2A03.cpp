#include "Apu2A03.h"

namespace
{
    constexpr uint8_t kDutyPatterns[4] = {
        0b00000010,
        0b00000110,
        0b00011110,
        0b11111001,
    };

    constexpr uint16_t kMinAudibleReload = 8;

    constexpr uint16_t kFrameStep1 = 3729;
    constexpr uint16_t kFrameStep2 = 7457;
    constexpr uint16_t kFrameStep3 = 11186;
    constexpr uint16_t kFrameStep4 = 14915;
}

void Apu2A03::PulseChannel::Reset()
{
    *this = PulseChannel{};
}

void Apu2A03::PulseChannel::LoadDutyPattern()
{
    sequencer.sequence = kDutyPatterns[dutyMode];
}

void Apu2A03::PulseChannel::WriteRegister(uint8_t index, uint8_t data)
{
    switch (index)
    {
    case 0:
    {
        const uint8_t newDuty = data >> 6;
        if (newDuty != dutyMode)
        {
            dutyMode = newDuty;
            LoadDutyPattern();
        }
        volume = data & 0x0F;
        break;
    }
    case 1:
        break;
    case 2:
        sequencer.reload = (sequencer.reload & 0x0700) | data;
        break;
    case 3:
        sequencer.reload = static_cast<uint16_t>(((data & 0x07) << 8) | (sequencer.reload & 0x00FF));
        sequencer.timer = sequencer.reload;
        LoadDutyPattern();
        break;
    }
}

void Apu2A03::PulseChannel::ClockTimer()
{
    sequencer.Clock(enabled, [](uint8_t& pattern)
    {
        pattern = static_cast<uint8_t>((pattern >> 1) | (pattern << 7));
    });
}

uint8_t Apu2A03::PulseChannel::Output() const
{
    if (!enabled || sequencer.reload < kMinAudibleReload)
        return 0;

    return sequencer.output ? volume : 0;
}

void Apu2A03::Reset()
{
    pulse1.Reset();
    pulse2.Reset();
    clockDivider = 0;
    frameClockCounter = 0;
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

    pulse1.ClockTimer();
    pulse2.ClockTimer();
}

double Apu2A03::GetOutputSample() const
{
    const double pulseLevel = pulse1.Output() + pulse2.Output();
    if (pulseLevel == 0.0)
        return 0.0;

    return 95.88 / (8128.0 / pulseLevel + 100.0);
}
