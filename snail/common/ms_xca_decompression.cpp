
#include <snail/common/ms_xca_decompression.hpp>

#include <algorithm>
#include <stdexcept>

#include <snail/common/parser/extract.hpp>

using namespace snail::common;

namespace {

// Plain LZ77 decompression algorithm according to the MS-XCA.
// See https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-xca

std::size_t decompress_xpress(std::span<const std::byte> input,
                              std::span<std::byte>       output)
{
    std::size_t in_offset = 0;
    std::size_t out_pos   = 0;

    int           flag_count = 0;
    std::uint32_t flags      = 0;

    std::size_t last_length_half_offset = 0;

    const auto input_size  = input.size();
    const auto output_size = output.size();

    while(in_offset < input_size)
    {
        if(flag_count == 0)
        {
            flags      = snail::common::parser::extract_move<std::uint32_t>(input, in_offset, std::endian::little);
            flag_count = 32;
        }

        --flag_count;

        if((flags & (1 << flag_count)) == 0)
        {
            if(out_pos >= output_size) throw std::runtime_error("Insufficient output buffer size");
            output[out_pos] = input[in_offset];
            ++in_offset;
            ++out_pos;
        }
        else
        {
            if(in_offset + 1 >= input_size) break;

            const auto match_bytes = snail::common::parser::extract_move<std::uint16_t>(input, in_offset, std::endian::little);

            auto       match_length = static_cast<std::uint32_t>(match_bytes % 8);
            const auto match_offset = static_cast<std::uint16_t>((match_bytes / 8) + 1);

            if(match_length == 7)
            {
                if(last_length_half_offset == 0)
                {
                    last_length_half_offset = in_offset;
                    match_length            = snail::common::parser::extract_move<std::uint8_t>(input, in_offset, std::endian::little);
                    match_length %= 16;
                }
                else
                {
                    match_length = snail::common::parser::extract<std::uint8_t>(input, last_length_half_offset, std::endian::little);
                    match_length /= 16;
                    last_length_half_offset = 0;
                }
                if(match_length == 15)
                {
                    match_length = snail::common::parser::extract_move<std::uint8_t>(input, in_offset, std::endian::little);
                    if(match_length == 255)
                    {
                        match_length = snail::common::parser::extract_move<std::uint16_t>(input, in_offset, std::endian::little);
                        if(match_length == 0)
                        {
                            match_length = snail::common::parser::extract_move<std::uint32_t>(input, in_offset, std::endian::little);
                        }
                        if(match_length < 15 + 7)
                        {
                            throw std::runtime_error("Invalid compressed data");
                        }
                        match_length -= (15 + 7);
                    }
                    match_length += 15;
                }
                match_length += 7;
            }
            match_length += 3;

            if(match_offset > out_pos) throw std::runtime_error("Invalid compressed data");
            if(out_pos + match_length > output_size) throw std::runtime_error("Insufficient output buffer size");
            for(std::uint32_t i = 0; i < match_length; ++i)
            {
                output[out_pos] = output[out_pos - match_offset];
                ++out_pos;
            }
        }
    }

    return out_pos;
}

} // namespace

std::size_t snail::common::ms_xca_decompress(std::span<const std::byte> input,
                                             std::span<std::byte>       output,
                                             ms_xca_compression_format  format)
{
    switch(format)
    {
    case ms_xca_compression_format::none:
        std::ranges::copy(input, output.begin());
        return input.size();
    case ms_xca_compression_format::lznt1:
    case ms_xca_compression_format::xpress_huff:
        throw std::runtime_error("Compression format not yet implemented");
    case ms_xca_compression_format::xpress:
        return decompress_xpress(input, output);
    default:
        throw std::runtime_error("Invalid compression format");
    }
}
