#pragma once

#include <array>
#include <cstddef>

#include "gba/types.hpp"

namespace gba {

class Cartridge;

class Memory {
public:
    explicit Memory(Cartridge& cartridge);

    u8  read8(u32 address) const;
    u16 read16(u32 address) const;
    u32 read32(u32 address) const;

    void write8(u32 address, u8 value);
    void write16(u32 address, u16 value);
    void write32(u32 address, u32 value);

private:
    Cartridge& cartridge_;

    std::array<u8, 256 * 1024> ewram_{};
    std::array<u8, 32 * 1024> iwram_{};
    std::array<u8, 1 * 1024> palette_ram_{};
    std::array<u8, 96 * 1024> vram_{};
    std::array<u8, 1 * 1024> oam_{};

    [[nodiscard]] u8 readCartridge8(u32 address) const;
};

} // namespace gba
