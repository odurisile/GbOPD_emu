#include "gba/cpu.hpp"

#include <bit>
#include <sstream>
#include <stdexcept>

#include "gba/memory.hpp"

namespace gba {
namespace {
constexpr u32 N = 1u << 31, Z = 1u << 30, C = 1u << 29, V = 1u << 28;
constexpr u32 T = 1u << 5;
int modeBank(u32 mode) {
    switch (mode) {
    case 0x10: case 0x1f: return 0;
    case 0x11: return 1;
    case 0x12: return 2;
    case 0x13: return 3;
    case 0x17: return 4;
    case 0x1b: return 5;
    default: return -1;
    }
}
u32 signExtend(u32 value, unsigned bits) {
    const u32 sign = 1u << (bits - 1);
    return (value ^ sign) - sign;
}
[[noreturn]] void unsupported(u32 instruction, u32 address) {
    std::ostringstream message;
    message << "Unsupported CPU instruction 0x" << std::hex << instruction
            << " at 0x" << address;
    throw std::runtime_error(message.str());
}
}

CPU::CPU(Memory& memory) : memory_(memory) { reset(); }

void CPU::reset(u32 entryPoint) {
    registers_.fill(0);
    banked_sp_lr_ = {};
    banked_high_ = {};
    // Direct entry without BIOS. Bit zero selects Thumb, as with BX.
    cpsr_ = 0x13 | ((entryPoint & 1) ? T : 0);
    writeRegister(15, entryPoint);
}

void CPU::step() {
    const u32 address = registers_[15];
    if (thumbMode()) {
        const u16 instruction = memory_.read16(address);
        registers_[15] += 2;
        executeThumb(instruction);
    } else {
        const u32 instruction = memory_.read32(address);
        registers_[15] += 4;
        executeArm(instruction);
    }
}
u32 CPU::pc() const noexcept { return registers_[15]; }
const std::array<u32, 16>& CPU::registers() const noexcept { return registers_; }
bool CPU::thumbMode() const noexcept { return (cpsr_ & T) != 0; }
u32 CPU::operand(unsigned reg) const noexcept {
    // step() already advanced by one instruction; visible PC is two ahead.
    return registers_[reg] + (reg == 15 ? (thumbMode() ? 2 : 4) : 0);
}
void CPU::writeRegister(unsigned reg, u32 value) {
    registers_[reg] = reg == 15 ? value & (thumbMode() ? ~1u : ~3u) : value;
}
void CPU::exchange(u32 address) {
    cpsr_ = (cpsr_ & ~T) | ((address & 1) ? T : 0);
    writeRegister(15, address);
}
void CPU::switchMode(u32 mode) {
    const u32 previous = cpsr_ & 31;
    if (previous == mode) return;
    const int oldBank = modeBank(previous), newBank = modeBank(mode);
    // Callers validate the new mode before modifying any CPU state.
    banked_sp_lr_[oldBank] = {registers_[13], registers_[14]};
    registers_[13] = banked_sp_lr_[newBank][0];
    registers_[14] = banked_sp_lr_[newBank][1];
    if ((previous == 0x11) != (mode == 0x11)) {
        for (unsigned r = 0; r < 5; ++r) {
            banked_high_[previous == 0x11][r] = registers_[r + 8];
            registers_[r + 8] = banked_high_[mode == 0x11][r];
        }
    }
    cpsr_ = (cpsr_ & ~31u) | mode;
}
void CPU::setNZ(u32 value) {
    cpsr_ = (cpsr_ & ~(N | Z)) | (value & N) | (value == 0 ? Z : 0);
}
u32 CPU::add(u32 a, u32 b, bool carry, bool flags) {
    const std::uint64_t sum = std::uint64_t(a) + b + unsigned(carry);
    const u32 result = static_cast<u32>(sum);
    if (flags) {
        setNZ(result);
        cpsr_ = (cpsr_ & ~(C | V)) | (sum >> 32 ? C : 0)
              | ((~(a ^ b) & (a ^ result) & N) ? V : 0);
    }
    return result;
}
bool CPU::conditionPassed(unsigned condition) const noexcept {
    const bool n = (cpsr_ & N) != 0, z = (cpsr_ & Z) != 0;
    const bool c = (cpsr_ & C) != 0, v = (cpsr_ & V) != 0;
    switch (condition) {
    case 0: return z; case 1: return !z;
    case 2: return c; case 3: return !c;
    case 4: return n; case 5: return !n;
    case 6: return v; case 7: return !v;
    case 8: return c && !z; case 9: return !c || z;
    case 10: return n == v; case 11: return n != v;
    case 12: return !z && n == v; case 13: return z || n != v;
    case 14: return true; default: return false;
    }
}
u32 CPU::shift(u32 value, unsigned type, unsigned amount, bool immediate, bool& carry) const {
    if (immediate && amount == 0) {
        if (type == 0) return value;
        if (type == 3) {
            const u32 result = (u32(carry) << 31) | (value >> 1);
            carry = (value & 1) != 0;
            return result;
        }
        amount = 32;
    }
    if (amount == 0) return value;
    switch (type) {
    case 0:
        carry = amount <= 32 && ((value >> (32 - amount)) & 1);
        return amount < 32 ? value << amount : 0;
    case 1:
        carry = amount <= 32 && ((value >> (amount - 1)) & 1);
        return amount < 32 ? value >> amount : 0;
    case 2:
        carry = amount >= 32 ? (value & N) != 0 : ((value >> (amount - 1)) & 1) != 0;
        if (amount >= 32) return (value & N) ? ~0u : 0;
        return (value >> amount) | ((value & N) ? (~0u << (32 - amount)) : 0);
    default:
        value = std::rotr(value, static_cast<int>(amount & 31));
        carry = (value & N) != 0;
        return value;
    }
}
u32 CPU::loadWord(u32 address) const {
    return std::rotr(memory_.read32(address & ~3u), static_cast<int>((address & 3) * 8));
}

void CPU::executeArm(u32 ins) {
    if (!conditionPassed(ins >> 28)) return;
    const unsigned rn = (ins >> 16) & 15, rd = (ins >> 12) & 15;
    if ((ins & 0x0ffffff0) == 0x012fff10) { exchange(operand(ins & 15)); return; }
    if ((ins & 0x0e000000) == 0x0a000000) {
        const u32 target = operand(15) + signExtend((ins & 0xffffff) << 2, 26);
        if (ins & (1u << 24)) registers_[14] = registers_[15];
        writeRegister(15, target);
        return;
    }
    if ((ins & 0x0fc000f0) == 0x00000090) {
        const unsigned dest = (ins >> 16) & 15;
        if (dest == 15 || (ins & 15) == 15 || ((ins >> 8) & 15) == 15)
            unsupported(ins, pc() - 4);
        u32 value = operand(ins & 15) * operand((ins >> 8) & 15);
        if (ins & (1u << 21)) value += operand(rd);
        registers_[dest] = value;
        if (ins & (1u << 20)) setNZ(value);
        return;
    }
    if ((ins & 0x0f8000f0) == 0x00800090) {
        const unsigned hi = rn, lo = rd, rm = ins & 15, rs = (ins >> 8) & 15;
        if (hi == 15 || lo == 15 || rm == 15 || rs == 15 || hi == lo) unsupported(ins, pc() - 4);
        std::uint64_t value;
        if (ins & (1u << 22)) {
            const auto a = std::int64_t(std::bit_cast<s32>(registers_[rm]));
            const auto b = std::int64_t(std::bit_cast<s32>(registers_[rs]));
            value = static_cast<std::uint64_t>(a * b);
        } else value = std::uint64_t(registers_[rm]) * registers_[rs];
        if (ins & (1u << 21)) value += (std::uint64_t(registers_[hi]) << 32) | registers_[lo];
        registers_[lo] = static_cast<u32>(value);
        registers_[hi] = static_cast<u32>(value >> 32);
        if (ins & (1u << 20)) cpsr_ = (cpsr_ & ~(N | Z)) | (registers_[hi] & N) | (value == 0 ? Z : 0);
        return;
    }
    if ((ins & 0x0fb00ff0) == 0x01000090) {
        const u32 address = operand(rn), value = operand(ins & 15);
        const bool byte = (ins & (1u << 22)) != 0;
        const u32 previous = byte ? memory_.read8(address) : loadWord(address);
        if (byte) memory_.write8(address, static_cast<u8>(value));
        else memory_.write32(address & ~3u, value);
        writeRegister(rd, previous);
        return;
    }
    if ((ins & 0x0e000090) == 0x00000090 && (ins & 0x60)) {
        const unsigned kind = (ins >> 5) & 3;
        const bool load = (ins & (1u << 20)) != 0;
        if ((!load && kind != 1) || rd == 15) unsupported(ins, pc() - 4);
        const u32 offset = (ins & (1u << 22)) ? ((ins >> 4) & 0xf0) | (ins & 15) : operand(ins & 15);
        const u32 base = operand(rn);
        const u32 adjusted = (ins & (1u << 23)) ? base + offset : base - offset;
        const bool pre = (ins & (1u << 24)) != 0;
        const u32 address = pre ? adjusted : base;
        u32 value = 0;
        if (!load) memory_.write16(address & ~1u, static_cast<u16>(operand(rd)));
        else if (kind == 1) value = std::rotr(u32(memory_.read16(address & ~1u)), int((address & 1) * 8));
        else if (kind == 2 || (address & 1)) value = signExtend(memory_.read8(address), 8);
        else value = signExtend(memory_.read16(address), 16);
        if (!pre || (ins & (1u << 21))) writeRegister(rn, adjusted);
        if (load) registers_[rd] = value;
        return;
    }
    if ((ins & 0x0fff0fff) == 0x010f0000) {
        if (rd == 15) unsupported(ins, pc() - 4);
        registers_[rd] = cpsr_;
        return;
    }
    if ((ins & 0x0ff0fff0) == 0x0120f000 || (ins & 0x0ff0f000) == 0x0320f000) {
        const unsigned fields = (ins >> 16) & 15;
        if (!(ins & (1u << 25)) && (ins & 15) == 15) unsupported(ins, pc() - 4);
        const u32 value = (ins & (1u << 25))
            ? std::rotr(ins & 255, int(((ins >> 8) & 15) * 2)) : operand(ins & 15);
        // User mode can only write flags. ARMv4T's reserved fields and T
        // remain unchanged; use BX to change instruction state.
        if ((fields & 1) && (cpsr_ & 31) != 0x10) {
            if (modeBank(value & 31) < 0) unsupported(ins, pc() - 4);
            switchMode(value & 31);
            cpsr_ = (cpsr_ & ~0xc0u) | (value & 0xc0);
        }
        if (fields & 8) cpsr_ = (cpsr_ & 0x0fffffff) | (value & 0xf0000000);
        return;
    }
    if ((ins & 0x0c000000) == 0x04000000) {
        if ((ins & 0x02000010) == 0x02000010) unsupported(ins, pc() - 4);
        u32 offset = ins & 0xfff;
        if (ins & (1u << 25)) {
            bool carry = (cpsr_ & C) != 0;
            offset = shift(operand(ins & 15), (ins >> 5) & 3, (ins >> 7) & 31, true, carry);
        }
        const u32 base = operand(rn);
        const u32 adjusted = (ins & (1u << 23)) ? base + offset : base - offset;
        const bool pre = (ins & (1u << 24)) != 0, load = (ins & (1u << 20)) != 0;
        const u32 address = pre ? adjusted : base;
        if (load) {
            const u32 value = (ins & (1u << 22)) ? memory_.read8(address) : loadWord(address);
            if (!pre || (ins & (1u << 21))) writeRegister(rn, adjusted);
            writeRegister(rd, value);
        } else {
            const u32 value = operand(rd) + (rd == 15 ? 4 : 0);
            if (ins & (1u << 22)) memory_.write8(address, static_cast<u8>(value));
            else memory_.write32(address & ~3u, value);
            if (!pre || (ins & (1u << 21))) writeRegister(rn, adjusted);
        }
        return;
    }
    if ((ins & 0x0e000000) == 0x08000000) {
        const u32 list = ins & 0xffff;
        if (!list || rn == 15 || (ins & (1u << 22))) unsupported(ins, pc() - 4);
        const u32 base = operand(rn), size = 4u * std::popcount(list);
        const bool up = (ins & (1u << 23)) != 0, pre = (ins & (1u << 24)) != 0;
        const bool load = (ins & (1u << 20)) != 0;
        u32 address = up ? base + (pre ? 4 : 0) : base - size + (pre ? 0 : 4);
        const u32 finalBase = up ? base + size : base - size;
        for (unsigned r = 0; r < 16; ++r) if (list & (1u << r)) {
            if (load) writeRegister(r, memory_.read32(address & ~3u));
            else {
                u32 value = operand(r) + (r == 15 ? 4 : 0);
                if (r == rn && (ins & (1u << 21)) && (list & ((1u << r) - 1))) value = finalBase;
                memory_.write32(address & ~3u, value);
            }
            address += 4;
        }
        if ((ins & (1u << 21)) && !(load && (list & (1u << rn)))) registers_[rn] = finalBase;
        return;
    }
    if ((ins & 0x0c000000) == 0) {
        // Reject miscellaneous encodings before the data-processing decoder.
        if (!(ins & (1u << 25)) && (ins & 0x90) == 0x90) unsupported(ins, pc() - 4);
        const unsigned op = (ins >> 21) & 15;
        const bool flags = (ins & (1u << 20)) != 0;
        if ((op >= 8 && op <= 11 && !flags) || (rd == 15 && flags)) unsupported(ins, pc() - 4);
        bool carry = (cpsr_ & C) != 0;
        const bool oldCarry = carry;
        u32 b, a = operand(rn);
        if (ins & (1u << 25)) {
            const unsigned rotate = ((ins >> 8) & 15) * 2;
            b = std::rotr(ins & 255, static_cast<int>(rotate));
            if (rotate) carry = (b & N) != 0;
        } else {
            const bool byRegister = (ins & 16) != 0;
            if (byRegister && (ins & 128)) unsupported(ins, pc() - 4);
            const unsigned rm = ins & 15;
            b = operand(rm) + (byRegister && rm == 15 ? 4 : 0);
            if (byRegister && rn == 15) a += 4;
            b = shift(b, (ins >> 5) & 3,
                      byRegister ? operand((ins >> 8) & 15) & 255 : (ins >> 7) & 31,
                      !byRegister, carry);
        }
        u32 result = 0;
        bool logical = false;
        switch (op) {
        case 0: case 8: result = a & b; logical = true; break;
        case 1: case 9: result = a ^ b; logical = true; break;
        case 2: case 10: result = add(a, ~b, true, flags); break;
        case 3: result = add(b, ~a, true, flags); break;
        case 4: case 11: result = add(a, b, false, flags); break;
        case 5: result = add(a, b, oldCarry, flags); break;
        case 6: result = add(a, ~b, oldCarry, flags); break;
        case 7: result = add(b, ~a, oldCarry, flags); break;
        case 12: result = a | b; logical = true; break;
        case 13: result = b; logical = true; break;
        case 14: result = a & ~b; logical = true; break;
        case 15: result = ~b; logical = true; break;
        }
        if (flags && logical) { setNZ(result); cpsr_ = (cpsr_ & ~C) | (carry ? C : 0); }
        if (op < 8 || op > 11) writeRegister(rd, result);
        return;
    }
    unsupported(ins, pc() - 4);
}

void CPU::executeThumb(u16 ins) {
    const unsigned rd = ins & 7, rs = (ins >> 3) & 7;
    if ((ins & 0xe000) == 0) {
        u32 result;
        if ((ins & 0x1800) == 0x1800) {
            const u32 b = (ins & 0x400) ? (ins >> 6) & 7 : registers_[(ins >> 6) & 7];
            result = (ins & 0x200) ? add(registers_[rs], ~b, true, true) : add(registers_[rs], b, false, true);
        } else {
            bool carry = (cpsr_ & C) != 0;
            result = shift(registers_[rs], (ins >> 11) & 3, (ins >> 6) & 31, true, carry);
            setNZ(result); cpsr_ = (cpsr_ & ~C) | (carry ? C : 0);
        }
        registers_[rd] = result;
        return;
    }
    if ((ins & 0xe000) == 0x2000) {
        const unsigned dest = (ins >> 8) & 7, op = (ins >> 11) & 3;
        const u32 immediate = ins & 255;
        if (op == 0) { registers_[dest] = immediate; setNZ(immediate); }
        else if (op == 1) add(registers_[dest], ~immediate, true, true);
        else registers_[dest] = op == 2 ? add(registers_[dest], immediate, false, true)
                                       : add(registers_[dest], ~immediate, true, true);
        return;
    }
    if ((ins & 0xfc00) == 0x4000) {
        const unsigned op = (ins >> 6) & 15;
        const u32 a = registers_[rd], b = registers_[rs];
        u32 result = a;
        bool carry = (cpsr_ & C) != 0;
        switch (op) {
        case 0: result = a & b; break; case 1: result = a ^ b; break;
        case 2: case 3: case 4: case 7:
            result = shift(a, op == 7 ? 3 : op - 2, b & 255, false, carry);
            cpsr_ = (cpsr_ & ~C) | (carry ? C : 0); break;
        case 5: result = add(a, b, carry, true); break;
        case 6: result = add(a, ~b, carry, true); break;
        case 8: setNZ(a & b); return;
        case 9: result = add(0, ~b, true, true); break;
        case 10: add(a, ~b, true, true); return;
        case 11: add(a, b, false, true); return;
        case 12: result = a | b; break; case 13: result = a * b; break;
        case 14: result = a & ~b; break; case 15: result = ~b; break;
        }
        registers_[rd] = result; setNZ(result); return;
    }
    if ((ins & 0xfc00) == 0x4400) {
        const unsigned dest = rd | ((ins >> 4) & 8), source = (ins >> 3) & 15;
        switch ((ins >> 8) & 3) {
        case 0: writeRegister(dest, operand(dest) + operand(source)); break;
        case 1: add(operand(dest), ~operand(source), true, true); break;
        case 2: writeRegister(dest, operand(source)); break;
        case 3:
            if (ins & 0x80) unsupported(ins, pc() - 2); // BLX is not ARMv4T.
            exchange(operand(source)); break;
        }
        return;
    }
    if ((ins & 0xf800) == 0x4800) {
        registers_[(ins >> 8) & 7] = loadWord((operand(15) & ~3u) + (ins & 255) * 4);
        return;
    }
    if ((ins & 0xf000) == 0x5000) {
        const u32 address = registers_[rs] + registers_[(ins >> 6) & 7];
        switch ((ins >> 9) & 7) {
        case 0: memory_.write32(address & ~3u, registers_[rd]); break;
        case 1: memory_.write16(address & ~1u, static_cast<u16>(registers_[rd])); break;
        case 2: memory_.write8(address, static_cast<u8>(registers_[rd])); break;
        case 3: registers_[rd] = signExtend(memory_.read8(address), 8); break;
        case 4: registers_[rd] = loadWord(address); break;
        case 5: registers_[rd] = std::rotr(u32(memory_.read16(address & ~1u)), int((address & 1) * 8)); break;
        case 6: registers_[rd] = memory_.read8(address); break;
        case 7: registers_[rd] = (address & 1) ? signExtend(memory_.read8(address), 8)
                                             : signExtend(memory_.read16(address), 16); break;
        }
        return;
    }
    if ((ins & 0xe000) == 0x6000 || (ins & 0xf000) == 0x8000) {
        const bool half = (ins & 0xf000) == 0x8000, byte = !half && (ins & 0x1000);
        const u32 address = registers_[rs] + ((ins >> 6) & 31) * (half ? 2u : byte ? 1u : 4u);
        if (ins & 0x800) registers_[rd] = half ? std::rotr(u32(memory_.read16(address & ~1u)), int((address & 1) * 8))
                                                            : byte ? memory_.read8(address) : loadWord(address);
        else if (half) memory_.write16(address & ~1u, static_cast<u16>(registers_[rd]));
        else if (byte) memory_.write8(address, static_cast<u8>(registers_[rd]));
        else memory_.write32(address & ~3u, registers_[rd]);
        return;
    }
    if ((ins & 0xf000) == 0x9000) {
        const unsigned dest = (ins >> 8) & 7;
        const u32 address = registers_[13] + (ins & 255) * 4;
        if (ins & 0x800) registers_[dest] = loadWord(address);
        else memory_.write32(address & ~3u, registers_[dest]);
        return;
    }
    if ((ins & 0xf000) == 0xa000) {
        registers_[(ins >> 8) & 7] = ((ins & 0x800) ? registers_[13] : operand(15) & ~3u) + (ins & 255) * 4;
        return;
    }
    if ((ins & 0xff00) == 0xb000) {
        const u32 offset = (ins & 127) * 4;
        registers_[13] += (ins & 128) ? 0u - offset : offset;
        return;
    }
    if ((ins & 0xf600) == 0xb400) {
        const bool pop = (ins & 0x800) != 0;
        const u32 list = (ins & 255) | ((ins & 0x100) ? (1u << (pop ? 15 : 14)) : 0);
        if (!list) unsupported(ins, pc() - 2);
        u32 address = registers_[13];
        if (!pop) address -= 4u * std::popcount(list);
        const u32 start = address;
        for (unsigned r = 0; r < 16; ++r) if (list & (1u << r)) {
            if (pop) writeRegister(r, memory_.read32(address & ~3u));
            else memory_.write32(address & ~3u, registers_[r]);
            address += 4;
        }
        registers_[13] = pop ? address : start;
        return;
    }
    if ((ins & 0xf000) == 0xc000) {
        const unsigned base = (ins >> 8) & 7;
        const u32 list = ins & 255;
        if (!list) unsupported(ins, pc() - 2);
        const bool load = (ins & 0x800) != 0;
        u32 address = registers_[base];
        const u32 end = address + 4u * std::popcount(list);
        for (unsigned r = 0; r < 8; ++r) if (list & (1u << r)) {
            if (load) registers_[r] = memory_.read32(address & ~3u);
            else memory_.write32(address & ~3u, r == base && (list & ((1u << r) - 1)) ? end : registers_[r]);
            address += 4;
        }
        if (!load || !(list & (1u << base))) registers_[base] = end;
        return;
    }
    if ((ins & 0xf000) == 0xd000) {
        const unsigned condition = (ins >> 8) & 15;
        if (condition >= 14) unsupported(ins, pc() - 2);
        if (conditionPassed(condition)) writeRegister(15, operand(15) + signExtend((ins & 255) << 1, 9));
        return;
    }
    if ((ins & 0xf800) == 0xe000) {
        writeRegister(15, operand(15) + signExtend((ins & 0x7ff) << 1, 12)); return;
    }
    if ((ins & 0xf800) == 0xf000) {
        registers_[14] = operand(15) + signExtend((ins & 0x7ff) << 12, 23); return;
    }
    if ((ins & 0xf800) == 0xf800) {
        const u32 target = registers_[14] + (ins & 0x7ff) * 2;
        registers_[14] = registers_[15] | 1;
        writeRegister(15, target); return;
    }
    unsupported(ins, pc() - 2);
}
} // namespace gba
