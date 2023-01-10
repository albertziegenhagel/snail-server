#pragma once

#include <concepts>
#include <type_traits>

#include "binarystream/binarystream.hpp"

namespace perfreader::etl::parser {

template<typename T>
    requires std::is_enum_v<T>
std::size_t parse(utility::binary_stream_parser& stream, T& data)
{
    std::underlying_type_t<T> underlying_type;
    const auto read_bytes = stream.read(underlying_type);
    data = static_cast<T>(underlying_type);
    return read_bytes;
}

std::size_t parse(utility::binary_stream_parser& stream, std::integral auto& data)
{
    return stream.read(data);
}

template<typename T, std::size_t N>
    requires std::is_integral_v<T>
std::size_t parse(utility::binary_stream_parser& stream, T (&data)[N])
{
    std::size_t read_bytes = 0;
    for(int i = 0; i < N; ++i)
    {
        read_bytes += stream.read(data[i]);
    }
    return read_bytes;
}

template<typename T>
T peek(utility::binary_stream_parser& stream)
{
    T result;
    const auto position = stream.tell_position();
    parse(stream, result);
    stream.set_position(position);
    return result;
}

} // perfreader::etl::parser