#include "gba/cartridge.hpp"

#include <fstream>
#include <iterator>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <Windows.h>

 char runtime_path_buffer[MAX_PATH];
 std::filesystem::path runtime_path;
 std::vector<uint8_t> cartridge_data;
 uint8_t gba_rom[GBA_ROM_SIZE];
 bool rom_loaded = false;
 cartridge_header_struct* gba_cartridge_header = (cartridge_header_struct*)(gba_rom + 0x00);

namespace gba {

bool Cartridge::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        return false;
    }

    rom_ = std::vector<u8>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );

    return !rom_.empty();
}

bool Cartridge::loaded() const noexcept {
    return !rom_.empty();
}

const std::vector<u8>& Cartridge::rom() const noexcept {
    return rom_;
}

bool cart_open_file(){
    if (!get_runtime_path())
    {
        return false;
    }

    char filename[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    ofn.lpstrFilter =
        "GBA ROM Files\0*.gba\0"
        "All Files\0*.*\0";

    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a GBA ROM file";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        return cart_load(filename);
    }

    return false;

}
void cart_print_info(){
    std::printf( "Entry point: %02X %02X %02X %02X\n",cartridge_data[0x00],cartridge_data[0x01],cartridge_data[0x02],cartridge_data[0x03]);
    std::printf("Title: ");

    for (int i = 0; i < 12; i++)
    {
        std::printf("%c", cartridge_data[0xA0 + i]);
    }
    
    std::printf("\n");
    std::printf("Game Code: %c%c%c%c\n",cartridge_data[0xAC],cartridge_data[0xAD],cartridge_data[0xAE],cartridge_data[0xAF]);
    std::printf("Maker Code: %c%c\n",cartridge_data[0xB0],cartridge_data[0xB1]);
    std::printf("Fixed Value: %02X\n",cartridge_data[0xB2]);
    std::printf("Software Version: %u\n",cartridge_data[0xBC]);
    std::printf("Header Checksum: %02X\n", cartridge_data[0xBD] );
    std::printf( "ROM Size: %zu bytes\n", cartridge_data.size());

}
bool cart_load(const char* filename)
{
    // Open the ROM file in binary mode and start at the end of the file.
    // Starting at the end lets us quickly determine the ROM's file size.
    std::ifstream file(
        filename,
        std::ios::binary | std::ios::ate
    );

    if (!file.is_open())
    {
        std::printf("Failed to load file %s\n", filename);
        return false;
    }

    std::streampos size = file.tellg();

    // Maximum GBA ROM size is 32 MB.
    constexpr size_t MAX_GBA_ROM_SIZE = 32 * 1024 * 1024;

    // Reject empty files or ROMs that are too large to be valid GBA ROMs.
    if (size <= 0 ||
        static_cast<size_t>(size) > MAX_GBA_ROM_SIZE)
    {
        std::printf("Invalid GBA ROM size\n");
        return false;
    }

    // Resize the cartridge vector so it has enough space to hold the entire ROM file.
    cartridge_data.resize(static_cast<size_t>(size));

    // Move the file cursor back to the beginning of the ROM.
    file.seekg(0, std::ios::beg);

    // Read the entire ROM file into cartridge_data. cartridge_data stores bytes, but ifstream::read expects char*, so we reinterpret the pointer.
    if (!file.read(
        reinterpret_cast<char*>(cartridge_data.data()),
        size))
    {
        std::printf("Failed to read ROM\n");
        return false;
    }

    // Print confirmation that the ROM was loaded successfully.
    std::printf(
        "ROM %s loaded, size: %lld bytes\n",
        filename,
        static_cast<long long>(size)
    );

    return true;
}

bool get_runtime_path() {
    std::error_code ec;

    runtime_path = std::filesystem::current_path(ec);

    if (ec)
    {
        return false;
    }

    return true;
}
} // namespace gba
