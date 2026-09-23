#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

#include "gba/emulator.hpp"

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
        std::cout << "Usage: gba <path/to/game.gba>\n"
                     "Load a ROM and execute the current 20-instruction debug run.\n";
        return 0;
    }
    if (argc != 2) {
        std::cerr << "Usage: gba <path/to/game.gba>\n";
        return 1;
    }

    try {
        // Keep the emulator's RAM arrays off the application's stack.
        auto emulator = std::make_unique<gba::Emulator>();
        const std::filesystem::path romPath(argv[1]);
        if (!emulator->loadRom(romPath)) {
            std::cerr << "Failed to load ROM: " << romPath << '\n';
            return 1;
        }

        emulator->reset();
        std::cout << "Loaded ROM: " << romPath << '\n';
        emulator->run();
        std::cout << "Completed the 20-instruction CPU debug run.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Emulation failed: " << error.what() << '\n';
        return 1;
    }
}
