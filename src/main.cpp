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

    return 0;
}
