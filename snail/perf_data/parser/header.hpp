
#pragma once

#include <cstdint>

#include <bitset>
#include <type_traits>

#include <iostream>

#include <snail/common/bit_flags.hpp>
#include <snail/common/parser/extract.hpp>

#include <snail/perf_data/parser/header_feature.hpp>

namespace snail::perf_data::parser {

struct file_section_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto offset() const { return extract<std::uint64_t>(0); }
    inline auto size() const { return extract<std::uint64_t>(8); }

    static inline constexpr std::size_t static_size = 16;
};

struct header_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto magic() const { return extract<std::uint64_t>(0); }

    inline auto size() const { return extract<std::uint64_t>(8); }
    inline auto attributes_size() const { return extract<std::uint64_t>(16); }

    inline auto attributes() const { return file_section_view(buffer().subspan(24), byte_order()); }
    inline auto data() const { return file_section_view(buffer().subspan(24 + file_section_view::static_size), byte_order()); }
    inline auto event_types() const { return file_section_view(buffer().subspan(24 + file_section_view::static_size * 2), byte_order()); }

    inline auto additional_features() const
    {
        common::bit_flags<header_feature, 256> flags;
        if(byte_order() != std::endian::native)
        {
            // We need to figure out the word size of the machine that wrote the file:
            // First we try 64bits and check whether header_feature::hostname is set
            // (which is supposed to be always set). If it is not set, we try it again
            // with 32bits.
            // Otherwise we unset all bits and set only header_feature::build_id.
            // This behaviour is the same as the one of the the original `perf` tool.
            extract_bitset<std::uint64_t>(24 + file_section_view::static_size * 3, flags.data());
            if(!flags.test(header_feature::hostname))
            {
                extract_bitset<std::uint32_t>(24 + file_section_view::static_size * 3, flags.data());
                if(!flags.test(header_feature::hostname))
                {
                    std::cout << "ERROR: unsupported byte order." << std::endl;
                    flags.reset();
                    flags.set(header_feature::build_id);
                }
            }
        }
        else
        {
            extract_bitset<std::uint64_t>(24 + file_section_view::static_size * 3, flags.data());
        }
        return flags;
    }

    static inline constexpr std::size_t static_size = 56 + file_section_view::static_size * 3;

private:
    template<typename WordType, std::size_t Bits>
    void extract_bitset(std::size_t offset, std::bitset<Bits>& flags) const
    {
        constexpr auto word_bits  = sizeof(WordType) * CHAR_BIT;
        constexpr auto word_count = (Bits + 1) / word_bits;

        for(std::size_t i = 0; i < word_count; ++i)
        {
            auto current = extract<WordType>(offset + i * sizeof(WordType));

            for(std::size_t bit = 0; bit < word_bits; ++bit)
            {
                const auto bit_offset = i * word_bits + bit;
                if(bit_offset >= Bits)
                    break;
                flags[bit_offset] = current & 1;
                current >>= 1;
            }
        }
    }
};

} // namespace snail::perf_data::parser
