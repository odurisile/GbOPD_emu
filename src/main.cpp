#include <iostream>

#include "gba/emulator.hpp"
#include "gba/cartridge.hpp"
#include "gba/memory.hpp"
#include "gba/cpu.hpp"
#include "gba/ppu.hpp"
#include "gba/timers.hpp"
#include <filesystem>
int main(int argc, char* argv[]) {
    // Path to the GBA ROM we want to test.
    const char* rom_path = "rom/PE.gba";

    // Load the ROM into cartridge memory.
    if (gba::cart_load(rom_path))
    {
        // Print information from the GBA cartridge header.
        gba::cart_print_info();
    }
    else
    {
        std::cerr << "Failed to load ROM\n";
        return 1;
    }

    return 0;
}
