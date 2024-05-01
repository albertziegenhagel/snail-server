
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
    static inline constexpr std::string_view event_name    = "PerfInfo-SampledProfile";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::perfinfo, 46, "SampledProfile"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto count() const { return extract<std::uint16_t>(dynamic_offset(4, 1)); }
    inline auto reserved() const { return extract<std::uint16_t>(dynamic_offset(6, 1)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(8, 1); }
};

// or `PmcCounterProfile:PerfInfo_V2` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v2_pmc_counter_profile_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "PerfInfo-PmcCounterProfile";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::perfinfo, 47, "PmcCounterProfile"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto instruction_pointer() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto profile_source() const { return extract<std::uint16_t>(dynamic_offset(4, 1)); }
    // inline auto reserved() const { return extract<std::uint16_t>(dynamic_offset(6, 1)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(8, 1); }
};

// or `PmcCounterConfig_V2:PerfInfo_V2` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v2_pmc_counter_config_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "PerfInfo-PmcCounterConfig";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::perfinfo, 48, "PmcCounterConfig"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto counter_count() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }

    inline std::u16string_view counter_name(std::size_t i) const
    {
        assert(i < counter_count());
        std::size_t names_offset = 0;
        for(std::size_t j = 0; j < i; ++j)
        {
            names_offset += counter_name(j).size() * 2 + 2;
        }
        if(counter_name_lengths.size() < counter_count()) counter_name_lengths.resize(counter_count());
        return extract_u16string(dynamic_offset(4, 0) + names_offset, counter_name_lengths[i]);
    }

    inline std::size_t dynamic_size() const
    {
        std::size_t names_offset = 0;
        for(std::size_t j = 0; j < counter_count(); ++j)
        {
            names_offset += counter_name(j).size() * 2 + 2;
        }
        return dynamic_offset(4 + names_offset, 0);
    }

private:
    mutable std::vector<std::optional<std::size_t>> counter_name_lengths;
};

// `SampledProfileInterval_V3:PerfInfo` from wmicore.mof in WDK 10.0.22621.0
struct perfinfo_v3_sampled_profile_interval_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "PerfInfo-SampledProfileInterval";
    static inline constexpr std::uint16_t    event_version = 3;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::perfinfo, 73, "CollectionStart"},
        event_identifier_group{event_trace_group::perfinfo, 74, "CollectionEnd"  }
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
