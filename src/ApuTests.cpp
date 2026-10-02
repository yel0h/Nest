#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "Apu2A03.h"
#include "Bus.h"
#include "Cartridge.h"

namespace
{
    int g_failures = 0;
    int g_checks = 0;

    void Check(bool condition, const char* expression, int line)
    {
        ++g_checks;
        if (condition)
            return;

        ++g_failures;
        std::cerr << "FAIL line " << line << ": " << expression << "\n";
    }

#define CHECK(expr) Check((expr), #expr, __LINE__)

    constexpr uint16_t kStatusRegister = 0x4015;

    std::string BuildFlatImage(uint8_t fill)
    {
        std::string image;
        image += "NES\x1A";
        image += static_cast<char>(2);
        image += static_cast<char>(1);
        image.append(10, '\0');

        std::string prg(32 * 1024, static_cast<char>(fill));
        prg[0x7FFC] = 0x00;
        prg[0x7FFD] = static_cast<char>(0x80);
        image += prg;
        image.append(8 * 1024, '\0');
        return image;
    }

    std::unique_ptr<Bus> MakeBus(uint8_t prgFill)
    {
        std::istringstream stream(BuildFlatImage(prgFill));
        auto bus = std::make_unique<Bus>();
        bus->InsertCartridge(std::make_shared<Cartridge>(stream));
        bus->Reset();
        return bus;
    }

    void ClockBus(Bus& bus, int systemClocks)
    {
        for (int i = 0; i < systemClocks; ++i)
            bus.Clock();
    }

    double PeakAfterRunning(Bus& bus, int systemClocks)
    {
        double peak = 0.0;
        for (int i = 0; i < systemClocks; ++i)
        {
            if (bus.Clock())
                peak = std::max(peak, bus.AudioSample());
        }
        return peak;
    }

    void TestRegisterDecoding()
    {
        Apu2A03 apu;
        apu.Reset();

        apu.CpuWrite(0x4012, 0x01);
        apu.CpuWrite(0x4013, 0x01);
        CHECK(!apu.DmcNeedsSample());
        CHECK((apu.CpuRead(kStatusRegister) & 0x10) == 0);

        apu.CpuWrite(kStatusRegister, 0x10);
        CHECK(apu.DmcNeedsSample());
        CHECK(apu.DmcSampleAddress() == 0xC040);
        CHECK((apu.CpuRead(kStatusRegister) & 0x10) != 0);

        apu.DmcReceiveSample(0x00);
        CHECK(apu.DmcSampleAddress() == 0xC041);
        CHECK(!apu.DmcNeedsSample());

        apu.CpuWrite(kStatusRegister, 0x00);
        CHECK((apu.CpuRead(kStatusRegister) & 0x10) == 0);
    }

    void TestLengthAndAddressWrap()
    {
        Apu2A03 apu;
        apu.Reset();

        apu.CpuWrite(0x4010, 0x0F);
        apu.CpuWrite(0x4012, 0xFF);
        apu.CpuWrite(0x4013, 0xFF);
        apu.CpuWrite(kStatusRegister, 0x10);
        CHECK(apu.DmcSampleAddress() == 0xFFC0);

        uint16_t lastAddress = 0;
        int fetched = 0;
        for (int guard = 0; guard < 200000 && fetched < 65; ++guard)
        {
            apu.Clock();
            if (!apu.DmcNeedsSample())
                continue;

            lastAddress = apu.DmcSampleAddress();
            apu.DmcReceiveSample(0x55);
            ++fetched;

            if (fetched == 64)
                CHECK(lastAddress == 0xFFFF);
            if (fetched == 65)
                CHECK(lastAddress == 0x8000);
        }
        CHECK(fetched == 65);
    }

    void TestIrqLifecycle()
    {
        auto bus = MakeBus(0x00);

        bus->CpuWrite(0x4010, 0x8F);
        bus->CpuWrite(0x4012, 0x00);
        bus->CpuWrite(0x4013, 0x00);
        bus->CpuWrite(kStatusRegister, 0x10);
        ClockBus(*bus, 300);

        const uint8_t status = bus->CpuRead(kStatusRegister);
        CHECK((status & 0x80) != 0);
        CHECK((status & 0x10) == 0);
        CHECK(bus->apu.IrqPending());

        bus->CpuRead(kStatusRegister);
        CHECK(bus->apu.IrqPending());

        bus->CpuWrite(kStatusRegister, 0x00);
        CHECK(!bus->apu.IrqPending());
        CHECK((bus->CpuRead(kStatusRegister) & 0x80) == 0);

        bus->CpuWrite(kStatusRegister, 0x10);
        ClockBus(*bus, 3000);
        CHECK(bus->apu.IrqPending());
        bus->CpuWrite(0x4010, 0x0F);
        CHECK(!bus->apu.IrqPending());
    }

    void TestNoIrqWhenDisabledOrLooping()
    {
        auto bus = MakeBus(0x00);

        bus->CpuWrite(0x4010, 0x0F);
        bus->CpuWrite(0x4013, 0x00);
        bus->CpuWrite(kStatusRegister, 0x10);
        ClockBus(*bus, 300);
        CHECK(!bus->apu.IrqPending());
        CHECK((bus->CpuRead(kStatusRegister) & 0x10) == 0);

        bus->CpuWrite(0x4010, 0xCF);
        bus->CpuWrite(kStatusRegister, 0x10);
        ClockBus(*bus, 100000);
        CHECK(!bus->apu.IrqPending());
        CHECK((bus->CpuRead(kStatusRegister) & 0x10) != 0);
    }

    void TestDeltaDirection()
    {
        auto rising = MakeBus(0xFF);
        rising->CpuWrite(0x4010, 0x4F);
        rising->CpuWrite(0x4011, 0x00);
        rising->CpuWrite(kStatusRegister, 0x10);
        const double risingPeak = PeakAfterRunning(*rising, 40000);

        auto falling = MakeBus(0x00);
        falling->CpuWrite(0x4010, 0x4F);
        falling->CpuWrite(0x4011, 0x00);
        falling->CpuWrite(kStatusRegister, 0x10);
        const double fallingPeak = PeakAfterRunning(*falling, 40000);

        CHECK(risingPeak > 0.01);
        CHECK(fallingPeak < 0.001);
    }

    void TestDirectLoad()
    {
        auto bus = MakeBus(0x00);
        PeakAfterRunning(*bus, 2000);

        bus->CpuWrite(0x4011, 0x7F);
        const double peak = PeakAfterRunning(*bus, 2000);
        CHECK(peak > 0.05);
    }

    void TestCpuStall()
    {
        constexpr int kClocks = 150000;

        auto quiet = MakeBus(0xEA);
        ClockBus(*quiet, kClocks);

        auto busy = MakeBus(0xEA);
        busy->CpuWrite(0x4010, 0x4F);
        busy->CpuWrite(kStatusRegister, 0x10);
        ClockBus(*busy, kClocks);

        const int quietPc = quiet->cpu.pc;
        const int busyPc = busy->cpu.pc;
        CHECK(quietPc - busyPc > 150);
        CHECK(quietPc - busyPc < 400);
    }
}

int main()
{
    TestRegisterDecoding();
    TestLengthAndAddressWrap();
    TestIrqLifecycle();
    TestNoIrqWhenDisabledOrLooping();
    TestDeltaDirection();
    TestDirectLoad();
    TestCpuStall();

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
