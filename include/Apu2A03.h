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
    struct PulseOscillator
    {
        double frequency = 0.0;
        double dutyCycle = 0.5;
        double amplitude = 1.0;

        double Sample(double time) const;
    };

    struct PulseChannel
    {
        bool enabled = false;

        uint8_t dutyMode = 0;
        uint8_t volume = 0;
        uint16_t timerReload = 0;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);

        double Output(double time) const;
    };

    static constexpr double kSystemClockHz = 5369318.0;
    static constexpr uint8_t kSystemClocksPerApuClock = 6;

    void ClockQuarterFrame();
    void ClockHalfFrame();

    PulseChannel pulse1;
    PulseChannel pulse2;

    uint8_t clockDivider = 0;
    uint16_t frameClockCounter = 0;

    double globalTime = 0.0;
};
