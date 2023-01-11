
#pragma once

#include <cstdint>

#include <array>

#include "etl/parser/extract.hpp"
#include "etl/parser/utility.hpp"

//
// event records for event_trace_group::perfinfo
//

namespace perfreader::etl::parser {

// See https://learn.microsoft.com/en-us/windows/win32/etw/sampledprofile
// or _PERFINFO_SAMPLED_PROFILE_INFORMATION in ntwmi.h
// or `SampledProfile:PerfInfo_V2` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v2_sampled_profile_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        sampled_profile = 46
    };
    static inline constexpr std::array<std::uint8_t, 1> event_types    = {46};
    static inline constexpr std::uint16_t                event_version = 2;

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto count() const { return extract<std::uint32_t>(dynamic_offset(4, 1)); }
};

} // namespace perfreader::etl::parser
