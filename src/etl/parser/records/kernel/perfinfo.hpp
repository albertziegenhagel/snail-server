
#pragma once

#include <cstdint>
#include <array>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::perfinfo

// See https://learn.microsoft.com/en-us/windows/win32/etw/sampledprofile
// or _PERFINFO_SAMPLED_PROFILE_INFORMATION in ntwmi.h
struct event_perfinfo_v2_sampled_profile
{
    enum class event_type : std::uint8_t
    {
        sampled_profile = 46
    };
    static inline constexpr std::array<std::uint8_t, 1> event_types     = {46};
    static inline constexpr std::array<std::uint16_t, 1> event_versions = {2};

    pointer instruction_pointer;
    std::uint32_t thread_id;
    std::uint32_t count;
};

std::size_t parse(utility::binary_stream_parser& stream, event_perfinfo_v2_sampled_profile& data, std::uint32_t pointer_size);

// `SampledProfile:PerfInfo_V2` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v2_sampled_profile_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        sampled_profile = 46
    };
    static inline constexpr std::array<std::uint8_t, 1> event_types     = {46};
    static inline constexpr std::array<std::uint16_t, 1> event_versions = {2};

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto count() const { return extract<std::uint32_t>(dynamic_offset(4, 1)); }
};

} // namespace perfreader::etl::parser
