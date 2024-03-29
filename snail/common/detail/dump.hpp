#pragma once

#include <cstdint>

#include <bit>
#include <format>
#include <iostream>
#include <span>

namespace snail::common::detail {

// this can be used to generate tests
inline void dump_buffer(std::span<const std::byte> buffer, std::size_t offset, std::size_t size, std::string_view name = "buffer")
{
    std::cout << "    const std::array<std::uint8_t, " << size << "> " << name << " = {\n";
    std::cout << "        ";
    for(std::size_t i = 0; i < size; ++i)
    {
        std::cout << std::format("{:#04x}", std::uint8_t(buffer[offset + i]));
        if(i + 1 < size)
        {
            std::cout << ",";
            if((i + 1) % 16 == 0) std::cout << "\n        ";
            else std::cout << " ";
        }
    }
    std::cout << " };" << std::endl;
}

} // namespace snail::common::detail
