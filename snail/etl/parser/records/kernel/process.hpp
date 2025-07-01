
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// event records for event_trace_group::process
//

namespace snail::etl::parser {

// See https://learn.microsoft.com/en-us/windows/win32/etw/process-typegroup1
// or `Process_V4_TypeGroup1:Process_V4` from wmicore.mof in WDK 10.0.22621.0
struct process_v4_type_group1_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Process-TypeGroup1";
    static inline constexpr std::uint16_t    event_version = 4;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::process, 1,  "Load"   },
        event_identifier_group{event_trace_group::process, 2,  "Unload" },
        event_identifier_group{event_trace_group::process, 3,  "DcStart"},
        event_identifier_group{event_trace_group::process, 4,  "DcEnd"  },
        event_identifier_group{event_trace_group::process, 39, "Defunct"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto unique_process_key() const { return extract_pointer(dynamic_offset(0, 0)); }

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto parent_id() const { return extract<std::uint32_t>(dynamic_offset(4, 1)); }
    inline auto session_id() const { return extract<std::uint32_t>(dynamic_offset(8, 1)); }

    inline auto exit_status() const { return extract<std::int32_t>(dynamic_offset(12, 1)); }

    inline auto directory_table_base() const { return extract_pointer(dynamic_offset(16, 1)); }

    inline auto flags() const { return extract<std::uint32_t>(dynamic_offset(16, 2)); }

    // See the explanation for the `Sid` extension in
    // https://learn.microsoft.com/en-us/windows/win32/etw/event-tracing-mof-qualifiers
    inline bool                         has_sid() const { return extract<std::uint32_t>(dynamic_offset(20, 2)) != 0; }
    inline std::array<std::uint64_t, 2> user_sid_token_user() const
    {
        return {extract_pointer(dynamic_offset(20, 2)), extract_pointer(dynamic_offset(20, 3))};
    }
    inline auto user_sid() const
    {
        assert(has_sid());
        return sid_view(buffer().subspan(dynamic_offset(20, 4)));
    }

    inline auto image_filename() const { return extract_string(dynamic_offset(20 + (has_sid() ? user_sid().dynamic_size() : 4), has_sid() ? 4 : 2), image_filename_length); }
    inline auto command_line() const { return extract_u16string(dynamic_offset(20 + (has_sid() ? user_sid().dynamic_size() : 4), has_sid() ? 4 : 2) + image_filename().size() + 1, command_line_length); }
    inline auto package_full_name() const { return extract_u16string(dynamic_offset(20 + (has_sid() ? user_sid().dynamic_size() : 4), has_sid() ? 4 : 2) + image_filename().size() + 1 + command_line().size() * 2 + 2, package_full_name_length); }
    inline auto application_id() const { return extract_u16string(dynamic_offset(20 + (has_sid() ? user_sid().dynamic_size() : 4), has_sid() ? 4 : 2) + image_filename().size() + 1 + command_line().size() * 2 + 2 + package_full_name().size() * 2 + 2, application_id_length); }

    inline std::size_t dynamic_size() const { return dynamic_offset(20 + (has_sid() ? user_sid().dynamic_size() : 4), has_sid() ? 4 : 2) + image_filename().size() + 1 + command_line().size() * 2 + 2 + package_full_name().size() * 2 + 2 + application_id().size() * 2 + 2; }

private:
    mutable std::optional<std::size_t> image_filename_length;
    mutable std::optional<std::size_t> command_line_length;
    mutable std::optional<std::size_t> package_full_name_length;
    mutable std::optional<std::size_t> application_id_length;
};

// See `Process_V2_TypeGroup2:Process_V2` from wmicore.mof in WDK 10.0.22621.0
struct process_v2_type_group2_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Process-TypeGroup2";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::process, 32, "PerformanceCounter"       },
        event_identifier_group{event_trace_group::process, 33, "PerformanceCounterRundown"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }

    inline auto page_fault_count() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }
    inline auto handle_count() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); }

    inline auto reserved() const { return extract<std::uint32_t>(dynamic_offset(12, 0)); }

    inline auto peak_virtual_size() const { return extract_pointer(dynamic_offset(16, 0)); }
    inline auto peak_working_set_size() const { return extract_pointer(dynamic_offset(16, 1)); }
    inline auto peak_pagefile_usage() const { return extract_pointer(dynamic_offset(16, 2)); }
    inline auto quota_peak_paged_pool_usage() const { return extract_pointer(dynamic_offset(16, 3)); }
    inline auto quota_peak_non_paged_pool_usage() const { return extract_pointer(dynamic_offset(16, 4)); }

    inline auto virtual_size() const { return extract_pointer(dynamic_offset(16, 5)); }
    inline auto working_set_size() const { return extract_pointer(dynamic_offset(16, 6)); }
    inline auto pagefile_usage() const { return extract_pointer(dynamic_offset(16, 7)); }
    inline auto quota_paged_pool_usage() const { return extract_pointer(dynamic_offset(16, 8)); }
    inline auto quota_non_paged_pool_usage() const { return extract_pointer(dynamic_offset(16, 9)); }

    inline auto private_page_count() const { return extract_pointer(dynamic_offset(16, 10)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(16, 11); }
};

} // namespace snail::etl::parser
