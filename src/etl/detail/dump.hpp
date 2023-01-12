#pragma once

#include <cstdint>

#include <iostream>
#include <span>
#include <format>
#include <bit>

namespace perfreader::etl::detail {

// this can be used to generate tests
void dump_buffer(std::span<const std::byte> buffer, std::size_t offset, std::size_t size)
{
    std::cout << "    const std::array<std::uint8_t, " << size << "> buffer = {\n";
    std::cout << "        ";
    for(std::size_t i = 0; i < size; ++i)
    {
        std::cout << std::format("{:#04x}", std::uint8_t(buffer[offset + i]));
        if(i+1 < size)
        {
            std::cout << ",";
            if((i+1) % 16 == 0) std::cout << "\n        ";
            else std::cout << " ";
        }
    }
    std::cout << " };" << std::endl;
}

} // perfreader::etl::detail
