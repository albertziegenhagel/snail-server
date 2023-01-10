
#pragma once

#include <cstdint>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::perfinfo

// See https://learn.microsoft.com/en-us/windows/win32/etw/sampledprofile
// or _PERFINFO_SAMPLED_PROFILE_INFORMATION in ntwmi.h
struct event_perfinfo_v2_sampled_profile
{
    static inline constexpr std::uint8_t event_type    = 46;
    static inline constexpr std::uint8_t event_version = 2;

    pointer instruction_pointer;
    std::uint32_t thread_id;
    std::uint32_t count;
};

std::size_t parse(utility::binary_stream_parser& stream, event_perfinfo_v2_sampled_profile& data);

} // namespace perfreader::etl::parser
