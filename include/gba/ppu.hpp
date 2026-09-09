#pragma once

#include <array>

#include "gba/types.hpp"

namespace gba {

class Memory;

class PPU {
public:
    static constexpr int Width = 240;
    static constexpr int Height = 160;

    explicit PPU(Memory& memory);

    void step(int cycles);

    [[nodiscard]] const std::array<u32, Width * Height>& framebuffer() const noexcept;

private:
    Memory& memory_;
    std::array<u32, Width * Height> framebuffer_{};

    int cycle_counter_ = 0;
};

} // namespace gba
