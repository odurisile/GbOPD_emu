#pragma once

#include <filesystem>
#include <vector>

#include "gba/types.hpp"

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
