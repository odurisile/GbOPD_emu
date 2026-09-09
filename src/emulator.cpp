#include "gba/emulator.hpp"

#include <iostream>

namespace gba {

Emulator::Emulator()
    : memory_(cartridge_),
      cpu_(memory_),
      ppu_(memory_) {}

bool Emulator::loadRom(const std::filesystem::path& path) {
    return cartridge_.load(path);
}

void Emulator::reset() {
    cpu_.reset();
}

void Emulator::run() {
    running_ = true;

    // Temporary debug loop.
    // Replace this with a real event/frame loop later.
    for (int i = 0; i < 20 && running_; ++i) {
        cpu_.step();

        // Placeholder cycle count until CPU instructions
        // return accurate timing information.
        constexpr int Cycles = 1;

        ppu_.step(Cycles);
        timers_.step(Cycles);
    }
}

void Emulator::runFrame() {
    // GBA CPU clock ~= 16.78 MHz
    // ~280,896 CPU cycles per video frame.
    constexpr int ApproxCyclesPerFrame = 280896;

    for (int cycles = 0; cycles < ApproxCyclesPerFrame;) {
        cpu_.step();

        // TODO: CPU::step() should eventually return
        // the real instruction cycle count.
        constexpr int InstructionCycles = 1;

        ppu_.step(InstructionCycles);
        timers_.step(InstructionCycles);

        cycles += InstructionCycles;
    }
}

} // namespace gba
