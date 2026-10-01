#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "Apu2A03.h"
#include "Cpu6502.h"
#include "Ppu2C02.h"

class Cartridge;

enum ControllerButton : uint8_t
{
    ControllerButtonA = 1 << 7,
    ControllerButtonB = 1 << 6,
    ControllerButtonSelect = 1 << 5,
    ControllerButtonStart = 1 << 4,
    ControllerButtonUp = 1 << 3,
    ControllerButtonDown = 1 << 2,
    ControllerButtonLeft = 1 << 1,
    ControllerButtonRight = 1 << 0,
};

class Bus
{
public:
    Bus();
    ~Bus();

    Cpu6502 cpu;
    Ppu2C02 ppu;
    Apu2A03 apu;
    std::array<uint8_t, 2 * 1024> cpuRam{};

    std::array<uint8_t, 2> controllerState{};

    void InsertCartridge(const std::shared_ptr<Cartridge>& cart);
    void Reset();

    void SetAudioSampleRate(uint32_t hz);
    bool Clock();
    double AudioSample() const { return audioSample; }

    void CpuWrite(uint16_t address, uint8_t data);
    uint8_t CpuRead(uint16_t address, bool readOnly = false);

private:
    void StepOamDma();

    std::shared_ptr<Cartridge> cartridge;
    uint32_t systemClockCounter = 0;

    static constexpr uint32_t kSystemClockHz = 5369318;
    uint32_t audioSampleRate = 44100;
    uint32_t audioPhase = 0;
    double audioSample = 0.0;

    std::array<uint8_t, 2> controllerShift{};

    bool oamDmaActive = false;
    bool oamDmaAwaitingSync = true;
    uint8_t oamDmaPage = 0x00;
    uint8_t oamDmaOffset = 0x00;
    uint8_t oamDmaLatch = 0x00;
};
