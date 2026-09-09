#include <iostream>

#include "gba/emulator.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: gba <rom.gba>\n";
        return 1;
    }

    gba::Emulator emulator;

    if (!emulator.loadRom(argv[1])) {
        std::cerr << "Failed to load ROM: " << argv[1] << '\n';
        return 1;
    }

    emulator.reset();
    emulator.run();

    return 0;
}
