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
    struct Sequencer
    {
        uint16_t timer = 0;
        uint16_t reload = 0;
        uint8_t sequence = 0;
        uint8_t output = 0;

        template <typename StepFn>
        uint8_t Clock(bool enable, StepFn&& step)
        {
            if (enable)
            {
                if (timer-- == 0)
                {
                    timer = reload;
                    output = sequence & 0x01;
                    step(sequence);
                }
            }
            return output;
        }
    };

    struct PulseChannel
    {
        bool enabled = false;

        uint8_t dutyMode = 0;
        uint8_t volume = 0;

        Sequencer sequencer;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);
        void ClockTimer();

        uint8_t Output() const;

    private:
        void LoadDutyPattern();
    };

    static constexpr uint8_t kSystemClocksPerApuClock = 6;

    void ClockQuarterFrame();
    void ClockHalfFrame();

    PulseChannel pulse1;

    uint8_t clockDivider = 0;
    uint16_t frameClockCounter = 0;
};
