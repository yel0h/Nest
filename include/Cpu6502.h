#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Bus;

class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();

    enum StatusFlag : uint8_t
    {
        Carry = 1 << 0,
        Zero = 1 << 1,
        InterruptOff= 1 << 2,
        Decimal = 1 << 3,
        Break = 1 << 4,
        Unused = 1 << 5,
        Overflow = 1 << 6,
        Negative = 1 << 7,
    };

    uint8_t a = 0x00;
    uint8_t x = 0x00;
    uint8_t y = 0x00;
    uint8_t sp = 0x00;
    uint16_t pc = 0x0000;
    uint8_t status = 0x00;

    void ConnectBus(Bus* target) { bus = target; }

    void Clock();
    void Reset();
    void Irq();
    void Nmi();

    bool GetFlag(StatusFlag flag) const;
    void SetFlag(StatusFlag flag, bool value);

    bool InstructionComplete() const { return cyclesRemaining == 0; }

    struct Instruction
    {
        std::string mnemonic;
        uint8_t (Cpu6502::*operate)() = nullptr;
        uint8_t (Cpu6502::*addrMode)() = nullptr;
        uint8_t baseCycles = 0;
    };
    const std::vector<Instruction>& Instructions() const { return instructionTable; }

private:
    Bus* bus = nullptr;

    uint8_t Read(uint16_t address) const;
    void Write(uint16_t address, uint8_t data);

    uint8_t AM_Implied();
    uint8_t AM_Immediate();
    uint8_t AM_ZeroPage();
    uint8_t AM_ZeroPageX();
    uint8_t AM_ZeroPageY();
    uint8_t AM_Relative();
    uint8_t AM_Absolute();
    uint8_t AM_AbsoluteX();
    uint8_t AM_AbsoluteY();
    uint8_t AM_Indirect();
    uint8_t AM_IndirectX();
    uint8_t AM_IndirectY();

    uint8_t OP_ADC(); uint8_t OP_AND(); uint8_t OP_ASL(); uint8_t OP_BCC();
    uint8_t OP_BCS(); uint8_t OP_BEQ(); uint8_t OP_BIT(); uint8_t OP_BMI();
    uint8_t OP_BNE(); uint8_t OP_BPL(); uint8_t OP_BRK(); uint8_t OP_BVC();
    uint8_t OP_BVS(); uint8_t OP_CLC(); uint8_t OP_CLD(); uint8_t OP_CLI();
    uint8_t OP_CLV(); uint8_t OP_CMP(); uint8_t OP_CPX(); uint8_t OP_CPY();
    uint8_t OP_DEC(); uint8_t OP_DEX(); uint8_t OP_DEY(); uint8_t OP_EOR();
    uint8_t OP_INC(); uint8_t OP_INX(); uint8_t OP_INY(); uint8_t OP_JMP();
    uint8_t OP_JSR(); uint8_t OP_LDA(); uint8_t OP_LDX(); uint8_t OP_LDY();
    uint8_t OP_LSR(); uint8_t OP_NOP(); uint8_t OP_ORA(); uint8_t OP_PHA();
    uint8_t OP_PHP(); uint8_t OP_PLA(); uint8_t OP_PLP(); uint8_t OP_ROL();
    uint8_t OP_ROR(); uint8_t OP_RTI(); uint8_t OP_RTS(); uint8_t OP_SBC();
    uint8_t OP_SEC(); uint8_t OP_SED(); uint8_t OP_SEI(); uint8_t OP_STA();
    uint8_t OP_STX(); uint8_t OP_STY(); uint8_t OP_TAX(); uint8_t OP_TAY();
    uint8_t OP_TSX(); uint8_t OP_TXA(); uint8_t OP_TXS(); uint8_t OP_TYA();

    uint8_t OP_XXX();

    std::vector<Instruction> instructionTable;

    uint8_t fetched = 0x00;
    uint16_t addrAbs = 0x0000;
    uint16_t addrRel = 0x0000;
    uint8_t opcode = 0x00;
    uint8_t cyclesRemaining = 0;

    uint8_t Fetch();
    void Branch(bool condition);
};
