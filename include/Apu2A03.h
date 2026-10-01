#pragma once

#include <cstdint>

class Apu2A03
{
public:
    void Reset();

    void CpuWrite(uint16_t address, uint8_t data);

    void Clock();

    double GetOutputSample() const;

private:
    struct PulseChannel
    {
        bool enabled = false;

        uint8_t dutyMode = 0;
        uint8_t volume = 0;

        uint16_t timerReload = 0;
        uint16_t timer = 0;
        uint8_t sequencePos = 0;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);
        void ClockTimer();

        uint8_t Output() const;
    };

    static constexpr uint8_t kSystemClocksPerApuClock = 6;

    PulseChannel pulse1;
    uint8_t clockDivider = 0;
};
