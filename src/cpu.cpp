#include "gba/cpu.hpp"

#include <iomanip>
#include <iostream>

#include "gba/memory.hpp"

namespace gba {

CPU::CPU(Memory& memory)
    : memory_(memory) {
    reset();
}

void CPU::reset() {
    registers_.fill(0);

    // Temporary direct-ROM start.
    // A more accurate emulator begins through BIOS/reset state.
    registers_[15] = 0x08000000;

    // Supervisor mode placeholder.
    cpsr_ = 0x00000013;
}

void CPU::step() {
    if (thumbMode()) {
        const u16 instruction = memory_.read16(registers_[15]);
        registers_[15] += 2;
        executeThumb(instruction);
    } else {
        const u32 instruction = memory_.read32(registers_[15]);
        registers_[15] += 4;
        executeArm(instruction);
    }
}

u32 CPU::pc() const noexcept {
    return registers_[15];
}

const std::array<u32, 16>& CPU::registers() const noexcept {
    return registers_;
}

bool CPU::thumbMode() const noexcept {
    constexpr u32 ThumbBit = 1u << 5;
    return (cpsr_ & ThumbBit) != 0;
}

void CPU::executeArm(u32 instruction) {
    // TODO:
    // 1. Check ARM condition code
    // 2. Decode instruction class
    // 3. Execute operation
    //
    // For now we only print fetched instructions.

    std::cout
        << "ARM  PC=0x"
        << std::hex << std::setw(8) << std::setfill('0')
        << (registers_[15] - 4)
        << " instruction=0x"
        << std::setw(8)
        << instruction
        << '\n';
}

void CPU::executeThumb(u16 instruction) {
    std::cout
        << "THUMB PC=0x"
        << std::hex << std::setw(8) << std::setfill('0')
        << (registers_[15] - 2)
        << " instruction=0x"
        << std::setw(4)
        << instruction
        << '\n';
}

} // namespace gba
