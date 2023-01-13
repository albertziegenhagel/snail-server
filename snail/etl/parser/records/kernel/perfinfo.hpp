
#pragma once

#include <cstdint>

#include <array>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/utility.hpp>
#include <snail/etl/parser/records/identifier.hpp>

//
// event records for event_trace_group::perfinfo
//

namespace snail::etl::parser {

// See https://learn.microsoft.com/en-us/windows/win32/etw/sampledprofile
// or _PERFINFO_SAMPLED_PROFILE_INFORMATION in ntwmi.h
// or `SampledProfile:PerfInfo_V2` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v2_sampled_profile_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{ event_trace_group::perfinfo, 46, "sampled_profile" }
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;
    using extract_view_dynamic_base::buffer;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto count() const { return extract<std::uint32_t>(dynamic_offset(4, 1)); }
};

} // namespace snail::etl::parser
