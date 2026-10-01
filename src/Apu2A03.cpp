#include "Apu2A03.h"

namespace
{
    constexpr uint8_t kDutyPatterns[4][8] = {
        {0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 1, 1, 1, 1},
    };

    constexpr uint16_t kMinAudibleReload = 8;
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
        sequencePos = 0;
        break;
    }
}

void Apu2A03::PulseChannel::ClockTimer()
{
    if (timer == 0)
    {
        timer = timerReload;
        sequencePos = (sequencePos + 1) & 0x07;
    }
    else
    {
        --timer;
    }
}

uint8_t Apu2A03::PulseChannel::Output() const
{
    if (!enabled || timerReload < kMinAudibleReload)
        return 0;

    return kDutyPatterns[dutyMode][sequencePos] ? volume : 0;
}

void Apu2A03::Reset()
{
    pulse1.Reset();
    clockDivider = 0;
}

void Apu2A03::CpuWrite(uint16_t address, uint8_t data)
{
    if (address >= 0x4000 && address <= 0x4003)
    {
        pulse1.WriteRegister(static_cast<uint8_t>(address & 0x0003), data);
    }
    else if (address == 0x4015)
    {
        pulse1.enabled = (data & 0x01) != 0;
    }
}

void Apu2A03::Clock()
{
    if (++clockDivider < kSystemClocksPerApuClock)
        return;

    clockDivider = 0;
    pulse1.ClockTimer();
}

double Apu2A03::GetOutputSample() const
{
    const double pulseLevel = pulse1.Output();
    if (pulseLevel == 0.0)
        return 0.0;

    return 95.88 / (8128.0 / pulseLevel + 100.0);
}
