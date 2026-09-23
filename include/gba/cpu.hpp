#pragma once

#include <array>

#include "gba/types.hpp"

namespace gba {

class Memory;

class CPU {
public:
    explicit CPU(Memory& memory);

    void reset(u32 entryPoint = 0x08000000);
    void step();

    [[nodiscard]] u32 pc() const noexcept;
    [[nodiscard]] const std::array<u32, 16>& registers() const noexcept;
    [[nodiscard]] u32 cpsr() const noexcept { return cpsr_; }

private:
    Memory& memory_;

    std::array<u32, 16> registers_{};
    u32 cpsr_ = 0;
    // User/System share a bank; FIQ additionally banks r8-r12.
    std::array<std::array<u32, 2>, 6> banked_sp_lr_{};
    std::array<std::array<u32, 5>, 2> banked_high_{};

    void executeArm(u32 instruction);
    void executeThumb(u16 instruction);

    [[nodiscard]] bool thumbMode() const noexcept;
    [[nodiscard]] bool conditionPassed(unsigned condition) const noexcept;
    [[nodiscard]] u32 operand(unsigned reg) const noexcept;
    void writeRegister(unsigned reg, u32 value);
    void exchange(u32 address);
    void switchMode(u32 mode);
    void setNZ(u32 value);
    u32 add(u32 a, u32 b, bool carry, bool flags);
    u32 shift(u32 value, unsigned type, unsigned amount, bool immediate, bool& carry) const;
    u32 loadWord(u32 address) const;
};

} // namespace gba
