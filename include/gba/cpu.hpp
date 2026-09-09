#pragma once

#include <array>

#include "gba/types.hpp"

namespace gba {

class Memory;

class CPU {
public:
    explicit CPU(Memory& memory);

    void reset();
    void step();

    [[nodiscard]] u32 pc() const noexcept;
    [[nodiscard]] const std::array<u32, 16>& registers() const noexcept;

private:
    Memory& memory_;

    std::array<u32, 16> registers_{};
    u32 cpsr_ = 0;

    void executeArm(u32 instruction);
    void executeThumb(u16 instruction);

    [[nodiscard]] bool thumbMode() const noexcept;
};

} // namespace gba
