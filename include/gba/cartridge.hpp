#pragma once

#include <filesystem>
#include <vector>

#include "gba/types.hpp"

const int GBA_ROM_SIZE = 0x2000000; // 32 MB
extern uint8_t gba_rom[GBA_ROM_SIZE];
extern bool rom_loaded;

struct cartridge_header_struct {
    uint8_t entry_point[4];     // 0x00 - ARM Branch instruction
    uint8_t nintendo_logo[156]; // 0x04 - Character graphic data
    uint8_t game_title[12];     // 0xA0 - Game Title (ASCII)
    uint8_t game_code[4];       // 0xAD - Game Code (ASCII)
    uint8_t maker_code[2];      // 0xAF - Maker Code (ASCII)
    uint8_t fixed_value;        // 0xB0 - Always 0x96
    uint8_t main_unit_code;     // 0xB1 - Target hardware
    uint8_t device_type;        // 0xB2 - Device attributes
    uint8_t reserved[9];        // 0xB3 - Reserved
    uint8_t software_version;   // 0xBD - Version number
    uint8_t header_checksum;    // 0xBE - Complement checksum
    uint8_t multiboot_flags[2]; // 0xBF - Multiboot details
    /* data */
};
extern cartridge_header_struct gba_cartridge_header;

bool cart_open_file();
void cart_print_info();
bool cart_load(const char* filename);

namespace gba {

class Cartridge {
public:
    bool load(const std::filesystem::path& path);

    [[nodiscard]] bool loaded() const noexcept;
    [[nodiscard]] const std::vector<u8>& rom() const noexcept;

private:
    std::vector<u8> rom_;
};

} // namespace gba
