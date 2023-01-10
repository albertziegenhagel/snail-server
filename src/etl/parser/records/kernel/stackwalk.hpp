
#pragma once

#include <cstdint>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::stackwalk

// or _STACK_WALK_EVENT_DATA in ntwmi.h
struct event_stackwalk_v0_data
{
    static inline constexpr std::uint8_t event_type    = 32;
    static inline constexpr std::uint8_t event_version = 0;

    std::uint64_t event_timestamp;
    std::uint32_t process_id;
    std::uint32_t thread_id;
    // std::vector<pointer> stack_addresses;
};

std::size_t parse(utility::binary_stream_parser& stream, event_stackwalk_v0_data& data);

} // namespace perfreader::etl::parser
