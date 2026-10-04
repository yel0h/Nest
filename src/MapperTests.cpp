#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Cartridge.h"
#include "Ppu2C02.h"

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

    std::string BuildImage(uint8_t mapper, uint8_t prg16, uint8_t chr8, uint8_t flags6Extra = 0)
    {
        std::string image;
        image += "NES\x1A";
        image += static_cast<char>(prg16);
        image += static_cast<char>(chr8);
        image += static_cast<char>(((mapper & 0x0F) << 4) | flags6Extra);
        image += static_cast<char>(mapper & 0xF0);
        image.append(8, '\0');

        for (unsigned unit = 0; unit < prg16 * 2u; ++unit)
            image.append(8 * 1024, static_cast<char>(unit));
        for (unsigned unit = 0; unit < chr8 * 8u; ++unit)
            image.append(1024, static_cast<char>(unit));

        return image;
    }

    std::unique_ptr<Cartridge> Load(const std::string& image)
    {
        std::istringstream stream(image);
        return std::make_unique<Cartridge>(stream);
    }

    uint8_t PrgAt(Cartridge& cart, uint16_t address)
    {
        uint8_t data = 0xEE;
        cart.CpuRead(address, data);
        return data;
    }

    uint8_t ChrAt(Cartridge& cart, uint16_t address)
    {
        uint8_t data = 0xEE;
        cart.PpuRead(address, data);
        return data;
    }

    void SerialWrite(Cartridge& cart, uint16_t address, uint8_t value)
    {
        for (int bit = 0; bit < 5; ++bit)
            cart.CpuWrite(address, (value >> bit) & 0x01);
    }

    void TestNrom()
    {
        auto cart = Load(BuildImage(0, 2, 1));
        CHECK(cart->ImageValid());
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(PrgAt(*cart, 0xE000) == 3);
        CHECK(ChrAt(*cart, 0x1C00) == 7);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);
    }

    void TestUnsupportedMapper()
    {
        auto cart = Load(BuildImage(5, 2, 1));
        CHECK(!cart->ImageValid());
    }

    void TestMmc1()
    {
        auto cart = Load(BuildImage(1, 8, 2));
        CHECK(cart->ImageValid());

        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(PrgAt(*cart, 0xC000) == 14);
        CHECK(PrgAt(*cart, 0xE000) == 15);

        SerialWrite(*cart, 0xE000, 3);
        CHECK(PrgAt(*cart, 0x8000) == 6);
        CHECK(PrgAt(*cart, 0xA000) == 7);
        CHECK(PrgAt(*cart, 0xC000) == 14);

        cart->CpuWrite(0xE000, 1);
        cart->CpuWrite(0xE000, 1);
        cart->CpuWrite(0xE000, 0x80);
        SerialWrite(*cart, 0xE000, 2);
        CHECK(PrgAt(*cart, 0x8000) == 4);

        SerialWrite(*cart, 0x8000, 0x0C);
        CHECK(cart->GetMirror() == Cartridge::Mirror::SingleScreenLow);
        SerialWrite(*cart, 0x8000, 0x0D);
        CHECK(cart->GetMirror() == Cartridge::Mirror::SingleScreenHigh);
        SerialWrite(*cart, 0x8000, 0x0E);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Vertical);
        SerialWrite(*cart, 0x8000, 0x0F);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);

        SerialWrite(*cart, 0x8000, 0x08);
        SerialWrite(*cart, 0xE000, 3);
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(PrgAt(*cart, 0xC000) == 6);

        SerialWrite(*cart, 0x8000, 0x00);
        SerialWrite(*cart, 0xE000, 5);
        CHECK(PrgAt(*cart, 0x8000) == 8);
        CHECK(PrgAt(*cart, 0xC000) == 10);

        SerialWrite(*cart, 0x8000, 0x0E);
        SerialWrite(*cart, 0xA000, 3);
        CHECK(ChrAt(*cart, 0x0000) == 8);
        CHECK(ChrAt(*cart, 0x1000) == 12);

        SerialWrite(*cart, 0x8000, 0x1E);
        SerialWrite(*cart, 0xA000, 3);
        SerialWrite(*cart, 0xC000, 2);
        CHECK(ChrAt(*cart, 0x0000) == 12);
        CHECK(ChrAt(*cart, 0x1000) == 8);
        CHECK(ChrAt(*cart, 0x13FF) == 8);

        CHECK(cart->CpuWrite(0x6000, 0xA5));
        CHECK(PrgAt(*cart, 0x6000) == 0xA5);
        CHECK(PrgAt(*cart, 0x7FFF) == 0x00);
    }

    void TestMmc1ChrRam()
    {
        auto cart = Load(BuildImage(1, 8, 0));
        CHECK(cart->PpuWrite(0x0010, 0x77));
        CHECK(ChrAt(*cart, 0x0010) == 0x77);

        SerialWrite(*cart, 0x8000, 0x1E);
        SerialWrite(*cart, 0xC000, 1);
        CHECK(cart->PpuWrite(0x1020, 0x42));
        CHECK(ChrAt(*cart, 0x1020) == 0x42);
    }

    void TestUxrom()
    {
        auto cart = Load(BuildImage(2, 4, 0));
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(PrgAt(*cart, 0xC000) == 6);

        cart->CpuWrite(0x8000, 2);
        CHECK(PrgAt(*cart, 0x8000) == 4);
        CHECK(PrgAt(*cart, 0xBFFF) == 5);
        CHECK(PrgAt(*cart, 0xC000) == 6);
        CHECK(PrgAt(*cart, 0xFFFF) == 7);

        CHECK(cart->PpuWrite(0x0100, 0x5A));
        CHECK(ChrAt(*cart, 0x0100) == 0x5A);
    }

    void TestCnrom()
    {
        auto cart = Load(BuildImage(3, 2, 4));
        CHECK(ChrAt(*cart, 0x0000) == 0);

        cart->CpuWrite(0x8000, 2);
        CHECK(ChrAt(*cart, 0x0000) == 16);
        CHECK(ChrAt(*cart, 0x1FFF) == 23);
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(!cart->PpuWrite(0x0000, 0x11));
    }

    void TestAxrom()
    {
        auto cart = Load(BuildImage(7, 8, 0));
        cart->CpuWrite(0x8000, 0x12);
        CHECK(PrgAt(*cart, 0x8000) == 8);
        CHECK(PrgAt(*cart, 0xE000) == 11);
        CHECK(cart->GetMirror() == Cartridge::Mirror::SingleScreenHigh);

        cart->CpuWrite(0x8000, 0x01);
        CHECK(PrgAt(*cart, 0x8000) == 4);
        CHECK(cart->GetMirror() == Cartridge::Mirror::SingleScreenLow);
    }

    void TestMmc2()
    {
        auto cart = Load(BuildImage(9, 8, 4));
        CHECK(cart->ImageValid());

        CHECK(PrgAt(*cart, 0xA000) == 13);
        CHECK(PrgAt(*cart, 0xC000) == 14);
        CHECK(PrgAt(*cart, 0xE000) == 15);

        cart->CpuWrite(0xA000, 0x03);
        CHECK(PrgAt(*cart, 0x8000) == 3);
        CHECK(PrgAt(*cart, 0xA000) == 13);

        cart->CpuWrite(0xB000, 1);
        cart->CpuWrite(0xC000, 2);
        cart->CpuWrite(0xD000, 3);
        cart->CpuWrite(0xE000, 4);

        uint64_t clock = 0;
        CHECK(ChrAt(*cart, 0x0000) == 8);
        CHECK(ChrAt(*cart, 0x1000) == 16);

        cart->NotifyPpuFetch(0x0FD8, clock++);
        CHECK(ChrAt(*cart, 0x0000) == 8);
        cart->NotifyPpuFetch(0x0000, clock++);
        CHECK(ChrAt(*cart, 0x0000) == 4);
        CHECK(ChrAt(*cart, 0x0FFF) == 7);

        cart->NotifyPpuFetch(0x1FE8, clock++);
        cart->NotifyPpuFetch(0x1000, clock++);
        CHECK(ChrAt(*cart, 0x1000) == 16);
        cart->NotifyPpuFetch(0x1FD8, clock++);
        cart->NotifyPpuFetch(0x1000, clock++);
        CHECK(ChrAt(*cart, 0x1000) == 12);
        CHECK(ChrAt(*cart, 0x0000) == 4);

        cart->NotifyPpuFetch(0x0FE8, clock++);
        cart->NotifyPpuFetch(0x0000, clock++);
        CHECK(ChrAt(*cart, 0x0000) == 8);

        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);
        cart->CpuWrite(0xF000, 0x00);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Vertical);
    }

    void TestColorDreams()
    {
        auto cart = Load(BuildImage(11, 4, 4));
        cart->CpuWrite(0x8000, 0x21);
        CHECK(PrgAt(*cart, 0x8000) == 4);
        CHECK(PrgAt(*cart, 0xFFFF) == 7);
        CHECK(ChrAt(*cart, 0x0000) == 16);
    }

    void TestGxrom()
    {
        auto cart = Load(BuildImage(66, 4, 4));
        cart->CpuWrite(0x8000, 0x11);
        CHECK(PrgAt(*cart, 0x8000) == 4);
        CHECK(ChrAt(*cart, 0x0000) == 8);
        CHECK(ChrAt(*cart, 0x1FFF) == 15);
    }

    void TestCamerica()
    {
        auto cart = Load(BuildImage(71, 8, 0));
        CHECK(PrgAt(*cart, 0xC000) == 14);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);

        cart->CpuWrite(0xC000, 0x03);
        CHECK(PrgAt(*cart, 0x8000) == 6);
        CHECK(PrgAt(*cart, 0xC000) == 14);

        cart->CpuWrite(0x9000, 0x10);
        CHECK(cart->GetMirror() == Cartridge::Mirror::SingleScreenHigh);

        CHECK(cart->PpuWrite(0x0200, 0x66));
        CHECK(ChrAt(*cart, 0x0200) == 0x66);
    }

    void TestNina()
    {
        auto cart = Load(BuildImage(79, 2, 4));
        CHECK(cart->CpuWrite(0x4100, 0x0B));
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(ChrAt(*cart, 0x0000) == 24);
        CHECK(!cart->CpuWrite(0x4000, 0x00));
        CHECK(ChrAt(*cart, 0x0000) == 24);
    }

    void TestUnromReversed()
    {
        auto cart = Load(BuildImage(180, 8, 0));
        cart->CpuWrite(0x8000, 0x05);
        CHECK(PrgAt(*cart, 0x8000) == 0);
        CHECK(PrgAt(*cart, 0xC000) == 10);
        CHECK(cart->PpuWrite(0x0010, 0x12));
        CHECK(ChrAt(*cart, 0x0010) == 0x12);
    }

    void RiseA12(Cartridge& cart, uint64_t& clock)
    {
        cart.NotifyPpuFetch(0x0000, clock);
        cart.NotifyPpuFetch(0x1000, clock + 20);
        clock += 100;
    }

    void TestMmc3Banking()
    {
        auto cart = Load(BuildImage(4, 8, 4));
        CHECK(cart->ImageValid());

        CHECK(PrgAt(*cart, 0xE000) == 15);
        CHECK(PrgAt(*cart, 0xC000) == 14);

        cart->CpuWrite(0x8000, 6);
        cart->CpuWrite(0x8001, 3);
        cart->CpuWrite(0x8000, 7);
        cart->CpuWrite(0x8001, 4);
        CHECK(PrgAt(*cart, 0x8000) == 3);
        CHECK(PrgAt(*cart, 0xA000) == 4);
        CHECK(PrgAt(*cart, 0xC000) == 14);

        cart->CpuWrite(0x8000, 0x46);
        CHECK(PrgAt(*cart, 0x8000) == 14);
        CHECK(PrgAt(*cart, 0xA000) == 4);
        CHECK(PrgAt(*cart, 0xC000) == 3);
        CHECK(PrgAt(*cart, 0xE000) == 15);

        const uint8_t chrRegisters[] = {6, 9, 20, 21, 22, 23};
        for (uint8_t reg = 0; reg < 6; ++reg)
        {
            cart->CpuWrite(0x8000, reg);
            cart->CpuWrite(0x8001, chrRegisters[reg]);
        }

        cart->CpuWrite(0x8000, 0x00);
        CHECK(ChrAt(*cart, 0x0000) == 6);
        CHECK(ChrAt(*cart, 0x0400) == 7);
        CHECK(ChrAt(*cart, 0x0800) == 8);
        CHECK(ChrAt(*cart, 0x0C00) == 9);
        CHECK(ChrAt(*cart, 0x1000) == 20);
        CHECK(ChrAt(*cart, 0x1400) == 21);
        CHECK(ChrAt(*cart, 0x1800) == 22);
        CHECK(ChrAt(*cart, 0x1C00) == 23);

        cart->CpuWrite(0x8000, 0x80);
        CHECK(ChrAt(*cart, 0x0000) == 20);
        CHECK(ChrAt(*cart, 0x0C00) == 23);
        CHECK(ChrAt(*cart, 0x1000) == 6);
        CHECK(ChrAt(*cart, 0x1400) == 7);
        CHECK(ChrAt(*cart, 0x1800) == 8);
        CHECK(ChrAt(*cart, 0x1C00) == 9);

        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);
        cart->CpuWrite(0xA000, 0x00);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Vertical);
        cart->CpuWrite(0xA000, 0x01);
        CHECK(cart->GetMirror() == Cartridge::Mirror::Horizontal);
    }

    void TestMmc3PrgRam()
    {
        auto cart = Load(BuildImage(4, 8, 4));

        cart->CpuWrite(0xA001, 0x80);
        cart->CpuWrite(0x6123, 0x33);
        CHECK(PrgAt(*cart, 0x6123) == 0x33);

        cart->CpuWrite(0xA001, 0xC0);
        cart->CpuWrite(0x6123, 0x44);
        CHECK(PrgAt(*cart, 0x6123) == 0x33);

        cart->CpuWrite(0xA001, 0x00);
        uint8_t data = 0;
        CHECK(!cart->CpuRead(0x6123, data));
    }

    void TestMmc3IrqCounter()
    {
        auto cart = Load(BuildImage(4, 8, 4));
        uint64_t clock = 1000;

        cart->CpuWrite(0xC000, 2);
        cart->CpuWrite(0xC001, 0);
        cart->CpuWrite(0xE001, 0);

        RiseA12(*cart, clock);
        RiseA12(*cart, clock);
        CHECK(!cart->IrqPending());
        RiseA12(*cart, clock);
        CHECK(cart->IrqPending());

        cart->CpuWrite(0xE000, 0);
        CHECK(!cart->IrqPending());

        cart->CpuWrite(0xC000, 0);
        cart->CpuWrite(0xC001, 0);
        cart->CpuWrite(0xE001, 0);
        cart->NotifyPpuFetch(0x1000, clock);
        cart->NotifyPpuFetch(0x0000, clock + 2);
        cart->NotifyPpuFetch(0x1000, clock + 4);
        CHECK(!cart->IrqPending());
        RiseA12(*cart, clock);
        CHECK(cart->IrqPending());

        cart->CpuWrite(0xE000, 0);
        RiseA12(*cart, clock);
        RiseA12(*cart, clock);
        CHECK(!cart->IrqPending());
    }

    void TestMmc3IrqFromPpu(uint8_t controlRegister, int riseCycle, const char* label)
    {
        std::shared_ptr<Cartridge> cart = Load(BuildImage(4, 8, 4));
        Ppu2C02 ppu;
        ppu.ConnectCartridge(cart);

        cart->CpuWrite(0xC000, 5);
        cart->CpuWrite(0xC001, 0);
        cart->CpuWrite(0xE001, 0);

        ppu.CpuWrite(0x2000, controlRegister);
        ppu.CpuWrite(0x2001, 0x18);

        int clocks = 0;
        while (!cart->IrqPending() && clocks < 341 * 262)
        {
            ppu.Clock();
            ++clocks;
        }

        const int expected = 5 * 341 + riseCycle + 1;
        if (clocks != expected)
            std::cerr << label << ": IRQ after " << clocks << " PPU clocks, expected " << expected << "\n";
        CHECK(clocks == expected);
    }

    void TestBatterySave()
    {
        const std::filesystem::path directory = std::filesystem::temp_directory_path();
        const std::filesystem::path romPath = directory / "nest_mapper_test_battery.nes";
        const std::filesystem::path savePath = directory / "nest_mapper_test_battery.sav";
        std::filesystem::remove(savePath);

        {
            std::ofstream rom(romPath, std::ios::binary);
            const std::string image = BuildImage(1, 8, 0, 0x02);
            rom.write(image.data(), static_cast<std::streamsize>(image.size()));
        }

        {
            Cartridge cart(romPath.string());
            CHECK(cart.ImageValid());
            CHECK(cart.HasBattery());
            CHECK(cart.CpuWrite(0x6010, 0xBE));
        }
        CHECK(std::filesystem::exists(savePath));

        {
            Cartridge cart(romPath.string());
            CHECK(PrgAt(cart, 0x6010) == 0xBE);
        }

        std::filesystem::remove(romPath);
        std::filesystem::remove(savePath);
    }
}

int main()
{
    TestNrom();
    TestUnsupportedMapper();
    TestMmc1();
    TestMmc1ChrRam();
    TestUxrom();
    TestCnrom();
    TestAxrom();
    TestMmc2();
    TestColorDreams();
    TestGxrom();
    TestCamerica();
    TestNina();
    TestUnromReversed();
    TestMmc3Banking();
    TestMmc3PrgRam();
    TestMmc3IrqCounter();
    TestMmc3IrqFromPpu(0x08, 261, "sprites at $1000");
    TestMmc3IrqFromPpu(0x10, 325, "background at $1000");
    TestBatterySave();

    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
