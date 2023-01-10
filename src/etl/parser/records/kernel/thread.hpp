
#pragma once

#include <cstdint>

#include <type_traits>
#include <string>

#include "binarystream/fwd/binarystream.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::thread

struct event_thread_XX
{
    static inline constexpr std::uint8_t event_type    = 0;
    static inline constexpr std::uint8_t event_version = 2;
};

std::size_t parse(utility::binary_stream_parser& stream, event_thread_XX& data);

} // namespace perfreader::etl::parser
