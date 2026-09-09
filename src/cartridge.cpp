#include "gba/cartridge.hpp"

#include <fstream>
#include <iterator>

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

} // namespace gba
