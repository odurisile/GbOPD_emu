#pragma once

#include <filesystem>

#include "gba/cartridge.hpp"
#include "gba/cpu.hpp"
#include "gba/memory.hpp"
#include "gba/ppu.hpp"
#include "gba/timers.hpp"

namespace gba {

class Emulator {
public:
    Emulator();

    bool loadRom(const std::filesystem::path& path);
    void reset();
    void run();
    void runFrame();

private:
    Cartridge cartridge_;
    Memory memory_;
    CPU cpu_;
    PPU ppu_;
    Timers timers_;

    bool running_ = false;
};

} // namespace gba
