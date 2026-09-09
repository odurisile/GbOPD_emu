#include "gba/ppu.hpp"

#include "gba/memory.hpp"

namespace gba {

PPU::PPU(Memory& memory)
    : memory_(memory) {
    framebuffer_.fill(0xFF000000);
}

void PPU::step(int cycles) {
    cycle_counter_ += cycles;

    // TODO:
    // Implement GBA scanline timing.
    // Eventually:
    // - HDraw
    // - HBlank
    // - VBlank
    // - DISPSTAT / VCOUNT
    // - background rendering
    // - sprites
}

const std::array<u32, PPU::Width * PPU::Height>&
PPU::framebuffer() const noexcept {
    return framebuffer_;
}

} // namespace gba
