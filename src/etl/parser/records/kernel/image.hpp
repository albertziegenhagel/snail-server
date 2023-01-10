
#pragma once

#include <cstdint>

#include <string>
#include <array>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// Event records for event_trace_group::image

// See https://learn.microsoft.com/en-us/windows/win32/etw/image-load
struct event_image_v2_load
{
    // 10=Load, 2=Unload, 3=DCStart, 4=DCEnd
    // 10=Load is in event_trace_group::process?? WHY?
    static inline constexpr std::array<std::uint8_t, 4> event_types = {2, 3, 4, 10};
    static inline constexpr std::uint8_t event_version              = 2;

    pointer ImageBase;
    pointer ImageSize;
    std::uint32_t ProcessId;
    std::uint32_t ImageCheckSum;
    std::uint32_t TimeDateStamp;
    std::uint32_t Reserved0;
    pointer DefaultBase;
    std::uint32_t Reserved1;
    std::uint32_t Reserved2;
    std::uint32_t Reserved3;
    std::uint32_t Reserved4;
    std::u16string FileName;
};

std::size_t parse(utility::binary_stream_parser& stream, event_image_v2_load& data);

} // namespace perfreader::etl::parser
