#pragma once

#include <bit>
#include <concepts>
#include <istream>
#include <string>
#include <vector>

namespace snail::common::parser {

template<typename T>
    requires std::is_integral_v<T>
inline T read_int(std::istream& stream, std::endian data_byte_order)
{
    T value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));

    if constexpr(sizeof(T) > 1)
    {
        if(data_byte_order != std::endian::native)
        {
            return std::byteswap(value);
        }

        return value;
    }
    else
    {
        return value;
    }
}

inline std::string read_string(std::istream& stream, std::endian data_byte_order)
{
    const auto length = read_int<std::uint32_t>(stream, data_byte_order);

    std::vector<char> buffer;
    buffer.resize(length + 1);
#if defined(__GNUC__) && !defined(__clang__) // workaround for invalid GCC diagnostic
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    buffer.back() = '\0'; // just to make sure we definitely have a valid string termination
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif

    stream.read(buffer.data(), length);

    return {buffer.data()};
}

inline std::vector<std::string> read_string_list(std::istream& stream, std::endian data_byte_order)
{
    const auto num_strings = read_int<std::uint32_t>(stream, data_byte_order);

    std::vector<std::string> result;
    result.reserve(num_strings);
    for(std::size_t i = 0; i < num_strings; ++i)
    {
        result.push_back(read_string(stream, data_byte_order));
    }
    return result;
}

} // namespace snail::common::parser
