
#pragma once

#include <cstdint>

#include <array>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

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
        event_identifier_group{event_trace_group::perfinfo, 46, "sample prof"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto count() const { return extract<std::uint16_t>(dynamic_offset(4, 1)); }
    inline auto reserved() const { return extract<std::uint16_t>(dynamic_offset(6, 1)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(8, 1); }
};

// `SampledProfileInterval_V3:PerfInfo` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v3_sampled_profile_interval_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 3;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::perfinfo, 73, "collection start"},
        event_identifier_group{event_trace_group::perfinfo, 74, "collection end"  }
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto source() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto new_interval() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }
    inline auto old_interval() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); }

    inline auto source_name() const { return extract_u16string(dynamic_offset(12, 0), source_name_length); }

    inline std::size_t dynamic_size() const { return dynamic_offset(12 + source_name().size() * 2 + 2, 0); }

private:
    mutable std::optional<std::size_t> source_name_length;
};

} // namespace snail::etl::parser
