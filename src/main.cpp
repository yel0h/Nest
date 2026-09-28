#include <cstdint>
#include <iostream>
#include <vector>

#include "Bus.h"

int main()
{
    Bus nes;

    const uint16_t loadAddress = 0x8000;
    const std::vector<uint8_t> program = {
        0xA9, 0x0A,
        0x8D, 0x00, 0x40,
        0xA9, 0x05,
        0x6D, 0x00, 0x40,
        0x8D, 0x01, 0x40,
        0x4C, 0x0D, 0x80,
    };

    for (size_t i = 0; i < program.size(); ++i)
        nes.Write(loadAddress + static_cast<uint16_t>(i), program[i]);

    nes.Write(0xFFFC, loadAddress & 0x00FF);
    nes.Write(0xFFFD, (loadAddress >> 8) & 0x00FF);

    nes.cpu.Reset();

    auto runOneInstruction = [&nes]()
    {
        do
        {
            nes.cpu.Clock();
        } while (!nes.cpu.InstructionComplete());
    };

    runOneInstruction();

    const int instructionsToRun = 5;
    for (int i = 0; i < instructionsToRun; ++i)
        runOneInstruction();

    std::cout << "A = " << static_cast<int>(nes.cpu.a) << " (expected 15)\n";
    std::cout << "mem[0x4000] = " << static_cast<int>(nes.Read(0x4000)) << " (expected 10)\n";
    std::cout << "mem[0x4001] = " << static_cast<int>(nes.Read(0x4001)) << " (expected 15)\n";
    std::cout << "Carry flag = " << nes.cpu.GetFlag(Cpu6502::Carry) << " (expected 0)\n";
    std::cout << "Zero flag = " << nes.cpu.GetFlag(Cpu6502::Zero) << " (expected 0)\n";
    std::cout << "Negative flag= " << nes.cpu.GetFlag(Cpu6502::Negative) << " (expected 0)\n";

    Bus stackTest;
    const std::vector<uint8_t> stackProgram = {
        0xA9, 0x50,
        0x38,
        0xE9, 0x10,
        0x48,
        0xA9, 0x00,
        0x68,
    };
    for (size_t i = 0; i < stackProgram.size(); ++i)
        stackTest.Write(loadAddress + static_cast<uint16_t>(i), stackProgram[i]);
    stackTest.Write(0xFFFC, loadAddress & 0x00FF);
    stackTest.Write(0xFFFD, (loadAddress >> 8) & 0x00FF);

    stackTest.cpu.Reset();
    auto runOne = [](Bus& b)
    {
        do
        {
            b.cpu.Clock();
        } while (!b.cpu.InstructionComplete());
    };
    runOne(stackTest);
    for (int i = 0; i < 6; ++i)
        runOne(stackTest);

    std::cout << "\nSBC/stack: A = " << static_cast<int>(stackTest.cpu.a) << " (expected 64)\n";
    std::cout << "SBC/stack: Carry flag = " << stackTest.cpu.GetFlag(Cpu6502::Carry) << " (expected 1)\n";
    std::cout << "SBC/stack: stack byte pushed by PHA = "
              << static_cast<int>(stackTest.Read(0x01FD)) << " (expected 64)\n";

    Bus interruptTest;
    const uint16_t handlerAddress = 0x9000;
    const std::vector<uint8_t> mainProgram = {
        0xA9, 0x07,
        0x8D, 0x00, 0x40,
    };
    const std::vector<uint8_t> handlerProgram = {
        0xEE, 0x20, 0x40,
        0x40,
    };
    for (size_t i = 0; i < mainProgram.size(); ++i)
        interruptTest.Write(loadAddress + static_cast<uint16_t>(i), mainProgram[i]);
    for (size_t i = 0; i < handlerProgram.size(); ++i)
        interruptTest.Write(handlerAddress + static_cast<uint16_t>(i), handlerProgram[i]);
    interruptTest.Write(0xFFFC, loadAddress & 0x00FF);
    interruptTest.Write(0xFFFD, (loadAddress >> 8) & 0x00FF);
    interruptTest.Write(0xFFFA, handlerAddress & 0x00FF);
    interruptTest.Write(0xFFFB, (handlerAddress >> 8) & 0x00FF);

    interruptTest.cpu.Reset();
    runOne(interruptTest);
    runOne(interruptTest);

    const bool interruptOffBeforeNmi = interruptTest.cpu.GetFlag(Cpu6502::InterruptOff);
    const uint8_t spBeforeNmi = interruptTest.cpu.sp;

    interruptTest.cpu.Nmi();
    runOne(interruptTest);
    runOne(interruptTest);
    runOne(interruptTest);
    runOne(interruptTest);

    std::cout << "\nNMI/RTI: mem[0x4020] = " << static_cast<int>(interruptTest.Read(0x4020)) << " (expected 1)\n";
    std::cout << "NMI/RTI: mem[0x4000] = " << static_cast<int>(interruptTest.Read(0x4000)) << " (expected 7)\n";
    std::cout << "NMI/RTI: stack pointer restored = " << (interruptTest.cpu.sp == spBeforeNmi) << " (expected 1)\n";
    std::cout << "NMI/RTI: InterruptOff restored = "
              << (interruptTest.cpu.GetFlag(Cpu6502::InterruptOff) == interruptOffBeforeNmi) << " (expected 1)\n";

    return 0;
}
