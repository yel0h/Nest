#pragma once

#include <cstdint>

class Apu2A03
{
public:
    void Reset();

    void CpuWrite(uint16_t address, uint8_t data);
    uint8_t CpuRead(uint16_t address);

    void Clock();

    double GetOutputSample();

    bool DmcNeedsSample() const { return dmc.NeedsSample(); }
    uint16_t DmcSampleAddress() const { return dmc.currentAddress; }
    void DmcReceiveSample(uint8_t data) { dmc.ReceiveSample(data); }

    bool IrqPending() const { return dmc.irqFlag; }

private:
    struct LengthCounter
    {
        uint8_t count = 0;
        bool halt = false;
        bool enabled = false;

        void Load(uint8_t tableIndex);
        void SetEnabled(bool value);
        void Clock();
    };

    struct Envelope
    {
        bool start = false;
        bool loop = false;
        bool constantVolume = false;
        uint8_t period = 0;
        uint8_t divider = 0;
        uint8_t decay = 0;

        void Clock();
        uint8_t Volume() const { return constantVolume ? period : decay; }
    };

    struct PulseOscillator
    {
        double frequency = 0.0;
        double dutyCycle = 0.5;
        double amplitude = 1.0;

        double Sample(double phase) const;
    };

    struct PulseChannel
    {
        explicit PulseChannel(bool isChannelOne = false) : channelOne(isChannelOne) {}

        bool channelOne;

        uint8_t dutyMode = 0;
        uint16_t timerReload = 0;
        double phase = 0.0;

        Envelope envelope;
        LengthCounter length;

        bool sweepEnabled = false;
        bool sweepNegate = false;
        bool sweepReload = false;
        uint8_t sweepPeriod = 0;
        uint8_t sweepShift = 0;
        uint8_t sweepDivider = 0;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);

        void ClockSweep();
        int SweepTarget() const;
        bool SweepSilences() const;

        double Output(double elapsedSeconds);
    };

    struct TriangleChannel
    {
        bool control = false;
        bool linearReloadFlag = false;
        uint8_t linearReload = 0;
        uint8_t linearCounter = 0;
        uint16_t timerReload = 0;
        double phase = 0.0;

        LengthCounter length;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);

        void ClockLinear();
        double Output(double elapsedSeconds);
    };

    struct NoiseChannel
    {
        Envelope envelope;
        LengthCounter length;

        bool shortMode = false;
        uint16_t shiftRegister = 1;
        uint32_t periodClocks = 0;
        uint32_t countdown = 0;

        double levelSum = 0.0;
        uint32_t levelSamples = 0;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);

        void ClockSystem();
        double Output();
    };

    struct DmcChannel
    {
        bool irqEnabled = false;
        bool loop = false;
        bool irqFlag = false;

        uint32_t periodClocks = 0;
        uint32_t countdown = 0;

        uint16_t sampleAddress = 0xC000;
        uint16_t sampleLength = 1;
        uint16_t currentAddress = 0xC000;
        uint16_t bytesRemaining = 0;

        uint8_t sampleBuffer = 0;
        bool bufferFilled = false;

        uint8_t shiftRegister = 0;
        uint8_t bitsRemaining = 8;
        bool silent = true;

        uint8_t outputLevel = 0;

        double levelSum = 0.0;
        uint32_t levelSamples = 0;

        void Reset();
        void WriteRegister(uint8_t index, uint8_t data);
        void SetEnabled(bool value);

        void RestartSample();
        bool NeedsSample() const { return !bufferFilled && bytesRemaining > 0; }
        void ReceiveSample(uint8_t data);

        void ClockSystem();
        void ClockOutputUnit();
        double Output();
    };

    static constexpr double kSystemClockHz = 5369318.0;
    static constexpr uint8_t kSystemClocksPerApuClock = 6;

    void ClockQuarterFrame();
    void ClockHalfFrame();

    PulseChannel pulse1{true};
    PulseChannel pulse2{false};
    TriangleChannel triangle;
    NoiseChannel noise;
    DmcChannel dmc;

    uint8_t clockDivider = 0;
    uint16_t frameClockCounter = 0;
    bool fiveStepMode = false;

    double secondsSinceLastSample = 0.0;

    double highPassInput = 0.0;
    double highPassOutput = 0.0;
    bool highPassPrimed = false;
};
