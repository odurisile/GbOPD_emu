#include "gba/memory.hpp"

#include "gba/cartridge.hpp"

namespace gba {

namespace {

template <std::size_t N>
u8 readArray(const std::array<u8, N>& data, u32 offset) {
    return data[offset % N];
}

template <std::size_t N>
void writeArray(std::array<u8, N>& data, u32 offset, u8 value) {
    data[offset % N] = value;
}

} // namespace

Memory::Memory(Cartridge& cartridge)
    : cartridge_(cartridge) {}

u8 Memory::read8(u32 address) const {
    switch (address >> 24) {
        case 0x02:
            return readArray(ewram_, address - 0x02000000);

        case 0x03:
            return readArray(iwram_, address - 0x03000000);

        case 0x05:
            return readArray(palette_ram_, address - 0x05000000);

        case 0x06:
            return readArray(vram_, address - 0x06000000);

        case 0x07:
            return readArray(oam_, address - 0x07000000);

        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return readCartridge8(address);

        default:
            // BIOS, IO registers, SRAM, and unmapped regions
            // will be added later.
            return 0;
    }
}

u16 Memory::read16(u32 address) const {
    const u16 lo = read8(address);
    const u16 hi = read8(address + 1);

    return static_cast<u16>(lo | (hi << 8));
}

u32 Memory::read32(u32 address) const {
    const u32 b0 = read8(address);
    const u32 b1 = read8(address + 1);
    const u32 b2 = read8(address + 2);
    const u32 b3 = read8(address + 3);

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Memory::write8(u32 address, u8 value) {
    switch (address >> 24) {
        case 0x02:
            writeArray(ewram_, address - 0x02000000, value);
            break;

        case 0x03:
            writeArray(iwram_, address - 0x03000000, value);
            break;

        case 0x05:
            writeArray(palette_ram_, address - 0x05000000, value);
            break;

        case 0x06:
            writeArray(vram_, address - 0x06000000, value);
            break;

        case 0x07:
            writeArray(oam_, address - 0x07000000, value);
            break;

        default:
            break;
    }
}

void Memory::write16(u32 address, u16 value) {
    write8(address, static_cast<u8>(value & 0xFF));
    write8(address + 1, static_cast<u8>((value >> 8) & 0xFF));
}

void Memory::write32(u32 address, u32 value) {
    write8(address, static_cast<u8>(value & 0xFF));
    write8(address + 1, static_cast<u8>((value >> 8) & 0xFF));
    write8(address + 2, static_cast<u8>((value >> 16) & 0xFF));
    write8(address + 3, static_cast<u8>((value >> 24) & 0xFF));
}

u8 Memory::readCartridge8(u32 address) const {
    if (!cartridge_.loaded()) {
        return 0;
    }

    const auto& rom = cartridge_.rom();
    const u32 offset = (address - 0x08000000) % static_cast<u32>(rom.size());

    return rom[offset];
}

} // namespace gba
