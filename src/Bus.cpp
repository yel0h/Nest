#include "Bus.h"

#include "Cartridge.h"

Bus::Bus()
{
    cpu.ConnectBus(this);
}

Bus::~Bus() = default;

void Bus::InsertCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
    ppu.ConnectCartridge(cart);
}

void Bus::Reset()
{
    cpu.Reset();
    apu.Reset();
    systemClockCounter = 0;
    audioPhase = 0;
}

void Bus::SetAudioSampleRate(uint32_t hz)
{
    if (hz == 0)
        return;

    audioSampleRate = hz;
    audioPhase = 0;
}

bool Bus::Clock()
{
    ppu.Clock();
    apu.Clock();

    if (systemClockCounter % 3 == 0)
    {
        if (oamDmaActive)
            StepOamDma();
        else
            cpu.Clock();
    }

    if (ppu.NmiRequested())
    {
        ppu.ClearNmiRequest();
        cpu.Nmi();
    }

    ++systemClockCounter;

    audioPhase += audioSampleRate;
    if (audioPhase < kSystemClockHz)
        return false;

    audioPhase -= kSystemClockHz;
    audioSample = apu.GetOutputSample();
    return true;
}

void Bus::StepOamDma()
{
    if (oamDmaAwaitingSync)
    {
        if (systemClockCounter % 2 != 0)
            oamDmaAwaitingSync = false;
        return;
    }

    const bool readCycle = (systemClockCounter % 2 == 0);
    if (readCycle)
    {
        const uint16_t sourceAddress = static_cast<uint16_t>((oamDmaPage << 8) | oamDmaOffset);
        oamDmaLatch = CpuRead(sourceAddress);
        return;
    }

    ppu.WriteOamByte(oamDmaOffset, oamDmaLatch);
    ++oamDmaOffset;

    if (oamDmaOffset == 0x00)
    {
        oamDmaActive = false;
        oamDmaAwaitingSync = true;
    }
}

void Bus::CpuWrite(uint16_t address, uint8_t data)
{
    if (cartridge && cartridge->CpuWrite(address, data))
        return;

    if (address <= 0x1FFF)
    {
        cpuRam[address & 0x07FF] = data;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        ppu.CpuWrite(address & 0x0007, data);
    }
    else if ((address >= 0x4000 && address <= 0x4013) || address == 0x4015 || address == 0x4017)
    {
        apu.CpuWrite(address, data);
    }
    else if (address == 0x4014)
    {
        oamDmaPage = data;
        oamDmaOffset = 0x00;
        oamDmaActive = true;
    }
    else if (address == 0x4016)
    {
        controllerShift = controllerState;
    }
}

uint8_t Bus::CpuRead(uint16_t address, bool readOnly)
{
    uint8_t data = 0x00;

    if (cartridge && cartridge->CpuRead(address, data))
        return data;

    if (address <= 0x1FFF)
    {
        data = cpuRam[address & 0x07FF];
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        data = ppu.CpuRead(address & 0x0007, readOnly);
    }
    else if (address == 0x4015)
    {
        data = apu.CpuRead(address);
    }
    else if (address == 0x4016 || address == 0x4017)
    {
        const uint8_t index = address & 0x0001;
        data = (controllerShift[index] & 0x80) ? 0x01 : 0x00;
        controllerShift[index] <<= 1;
    }

    return data;
}
