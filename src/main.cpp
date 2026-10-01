#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "Bus.h"
#include "Cartridge.h"

namespace
{
    void LoadProgram(Bus& nes, uint16_t address, const std::vector<uint8_t>& program)
    {
        for (size_t i = 0; i < program.size(); ++i)
            nes.CpuWrite(address + static_cast<uint16_t>(i), program[i]);
    }

    void SetVector(Bus& nes, uint16_t vectorAddress, uint16_t target)
    {
        nes.CpuWrite(vectorAddress, target & 0x00FF);
        nes.CpuWrite(vectorAddress + 1, (target >> 8) & 0x00FF);
    }

    void RunOneInstruction(Bus& nes)
    {
        do
        {
            nes.cpu.Clock();
        } while (!nes.cpu.InstructionComplete());
    }
}

int main()
{
    const uint16_t loadAddress = 0x8000;

    auto nes = std::make_unique<Bus>();
    nes->InsertCartridge(std::make_shared<Cartridge>());

    const std::vector<uint8_t> program = {
        0xA9, 0x0A,
        0x8D, 0x00, 0x03,
        0xA9, 0x05,
        0x6D, 0x00, 0x03,
        0x8D, 0x01, 0x03,
        0x4C, 0x0D, 0x80,
    };
    LoadProgram(*nes, loadAddress, program);
    SetVector(*nes, 0xFFFC, loadAddress);

    nes->cpu.Reset();

    RunOneInstruction(*nes);
    const int instructionsToRun = 5;
    for (int i = 0; i < instructionsToRun; ++i)
        RunOneInstruction(*nes);

    std::cout << "A = " << static_cast<int>(nes->cpu.a) << " (expected 15)\n";
    std::cout << "mem[0x0300] = " << static_cast<int>(nes->CpuRead(0x0300)) << " (expected 10)\n";
    std::cout << "mem[0x0301] = " << static_cast<int>(nes->CpuRead(0x0301)) << " (expected 15)\n";
    std::cout << "Carry flag = " << nes->cpu.GetFlag(Cpu6502::Carry) << " (expected 0)\n";
    std::cout << "Zero flag = " << nes->cpu.GetFlag(Cpu6502::Zero) << " (expected 0)\n";
    std::cout << "Negative flag= " << nes->cpu.GetFlag(Cpu6502::Negative) << " (expected 0)\n";

    auto stackTest = std::make_unique<Bus>();
    stackTest->InsertCartridge(std::make_shared<Cartridge>());

    const std::vector<uint8_t> stackProgram = {
        0xA9, 0x50,
        0x38,
        0xE9, 0x10,
        0x48,
        0xA9, 0x00,
        0x68,
    };
    LoadProgram(*stackTest, loadAddress, stackProgram);
    SetVector(*stackTest, 0xFFFC, loadAddress);

    stackTest->cpu.Reset();
    RunOneInstruction(*stackTest);
    for (int i = 0; i < 6; ++i)
        RunOneInstruction(*stackTest);

    std::cout << "\nSBC/stack: A = " << static_cast<int>(stackTest->cpu.a) << " (expected 64)\n";
    std::cout << "SBC/stack: Carry flag = " << stackTest->cpu.GetFlag(Cpu6502::Carry) << " (expected 1)\n";
    std::cout << "SBC/stack: stack byte pushed by PHA = "
              << static_cast<int>(stackTest->CpuRead(0x01FD)) << " (expected 64)\n";

    auto interruptTest = std::make_unique<Bus>();
    interruptTest->InsertCartridge(std::make_shared<Cartridge>());

    const uint16_t handlerAddress = 0x9000;
    const std::vector<uint8_t> mainProgram = {
        0xA9, 0x07,
        0x8D, 0x00, 0x03,
    };
    const std::vector<uint8_t> handlerProgram = {
        0xEE, 0x10, 0x03,
        0x40,
    };
    LoadProgram(*interruptTest, loadAddress, mainProgram);
    LoadProgram(*interruptTest, handlerAddress, handlerProgram);
    SetVector(*interruptTest, 0xFFFC, loadAddress);
    SetVector(*interruptTest, 0xFFFA, handlerAddress);

    interruptTest->cpu.Reset();
    RunOneInstruction(*interruptTest);
    RunOneInstruction(*interruptTest);

    const bool interruptOffBeforeNmi = interruptTest->cpu.GetFlag(Cpu6502::InterruptOff);
    const uint8_t spBeforeNmi = interruptTest->cpu.sp;

    interruptTest->cpu.Nmi();
    RunOneInstruction(*interruptTest);
    RunOneInstruction(*interruptTest);
    RunOneInstruction(*interruptTest);
    RunOneInstruction(*interruptTest);

    std::cout << "\nNMI/RTI: mem[0x0310] = " << static_cast<int>(interruptTest->CpuRead(0x0310)) << " (expected 1)\n";
    std::cout << "NMI/RTI: mem[0x0300] = " << static_cast<int>(interruptTest->CpuRead(0x0300)) << " (expected 7)\n";
    std::cout << "NMI/RTI: stack pointer restored = " << (interruptTest->cpu.sp == spBeforeNmi) << " (expected 1)\n";
    std::cout << "NMI/RTI: InterruptOff restored = "
              << (interruptTest->cpu.GetFlag(Cpu6502::InterruptOff) == interruptOffBeforeNmi) << " (expected 1)\n";

    auto branchTest = std::make_unique<Bus>();
    branchTest->InsertCartridge(std::make_shared<Cartridge>());

    const std::vector<uint8_t> branchProgram = {
        0xA2, 0x05,
        0xA9, 0x00,
        0x18,
        0x69, 0x01,
        0xCA,
        0xD0, 0xFA,
        0x8D, 0x00, 0x04,
    };
    LoadProgram(*branchTest, loadAddress, branchProgram);
    SetVector(*branchTest, 0xFFFC, loadAddress);

    branchTest->cpu.Reset();
    RunOneInstruction(*branchTest);
    const int branchInstructionsToRun = 23;
    for (int i = 0; i < branchInstructionsToRun; ++i)
        RunOneInstruction(*branchTest);

    std::cout << "\nBackward BNE loop: A = " << static_cast<int>(branchTest->cpu.a) << " (expected 5)\n";
    std::cout << "Backward BNE loop: mem[0x0400] = " << static_cast<int>(branchTest->CpuRead(0x0400)) << " (expected 5)\n";
    std::cout << "Backward BNE loop: PC after loop = 0x" << std::hex << branchTest->cpu.pc << std::dec
              << " (expected 0x800d)\n";

    auto apuTest = std::make_unique<Bus>();
    apuTest->InsertCartridge(std::make_shared<Cartridge>());
    apuTest->SetAudioSampleRate(44100);

    auto collectSamples = [&](int sampleCount, double& minOut, double& maxOut)
    {
        minOut = 1e9;
        maxOut = -1e9;
        for (int collected = 0; collected < sampleCount;)
        {
            if (!apuTest->Clock())
                continue;
            minOut = std::min(minOut, apuTest->AudioSample());
            maxOut = std::max(maxOut, apuTest->AudioSample());
            ++collected;
        }
    };

    double lowest = 0.0;
    double highest = 0.0;

    apuTest->CpuWrite(0x4000, 0xBF);
    apuTest->CpuWrite(0x4002, 0xFD);
    apuTest->CpuWrite(0x4003, 0x00);
    collectSamples(2000, lowest, highest);
    std::cout << "\nAPU channel disabled: max sample = " << highest << " (expected 0)\n";

    apuTest->CpuWrite(0x4015, 0x01);
    collectSamples(2000, lowest, highest);
    std::cout << "APU pulse 1 enabled: swings " << lowest << " .. " << highest << " (expected 0 .. >0)\n";

    apuTest->CpuWrite(0x4015, 0x00);
    collectSamples(2000, lowest, highest);
    std::cout << "APU pulse 1 disabled again: max sample = " << highest << " (expected 0)\n";

    return 0;
}
