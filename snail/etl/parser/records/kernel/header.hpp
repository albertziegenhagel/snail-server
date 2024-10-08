
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/log_file_mode.hpp>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// event records for event_trace_group::header
//

namespace snail::etl::parser {

// `EventTrace_Header:EventTraceEvent` from wmicore.mof in WDK 10.0.22621.0
struct event_trace_v2_header_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "EventTrace-Header";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::header, 0, "Header"}
    };

    inline explicit event_trace_v2_header_event_view(std::span<const std::byte> buffer) :
        extract_view_dynamic_base(buffer, common::parser::extract<std::uint32_t>(buffer, 44, etl_file_byte_order))
    {}

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    // start_time, end_time and boot_time are 100 nanosecond intervals since midnight, January 1, 1601.

    inline auto buffer_size() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto os_version_major() const { return extract<std::uint8_t>(dynamic_offset(4, 0)); }
    inline auto os_version_minor() const { return extract<std::uint8_t>(dynamic_offset(5, 0)); }
    inline auto sp_version_major() const { return extract<std::uint8_t>(dynamic_offset(6, 0)); }
    inline auto sp_version_minor() const { return extract<std::uint8_t>(dynamic_offset(7, 0)); }
    inline auto provider_version() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); } // OS build number
    inline auto number_of_processors() const { return extract<std::uint32_t>(dynamic_offset(12, 0)); }
    inline auto end_time() const { return extract<std::uint64_t>(dynamic_offset(16, 0)); }
    inline auto timer_resolution() const { return extract<std::uint32_t>(dynamic_offset(24, 0)); }
    inline auto max_file_size() const { return extract<std::uint32_t>(dynamic_offset(28, 0)); }
    inline auto log_file_mode() const { return log_file_mode_flags(extract<std::uint32_t>(dynamic_offset(32, 0))); }
    inline auto buffers_written() const { return extract<std::uint32_t>(dynamic_offset(36, 0)); }
    inline auto start_buffers() const { return extract<std::uint32_t>(dynamic_offset(40, 0)); }
    inline auto pointer_size() const { return extract<std::uint32_t>(dynamic_offset(44, 0)); }
    inline auto events_lost() const { return extract<std::uint32_t>(dynamic_offset(48, 0)); }
    inline auto cpu_speed() const { return extract<std::uint32_t>(dynamic_offset(52, 0)); }
    inline auto logger_name() const { return extract_pointer(dynamic_offset(56, 0)); }
    inline auto log_file_name() const { return extract_pointer(dynamic_offset(56, 1)); }
    inline auto time_zone_information() const { return time_zone_information_view(buffer().subspan(dynamic_offset(56, 2))); }
    // NOTE: for some reason the size of `time_zone_information` is given as 176 bytes in wmicore.mof
    //       but our data structure is only 172 bytes long. Hence we add an additional 4 bytes to the
    //       offset in the following.
    inline auto boot_time() const { return extract<std::uint64_t>(dynamic_offset(56 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto perf_freq() const { return extract<std::uint64_t>(dynamic_offset(64 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto start_time() const { return extract<std::uint64_t>(dynamic_offset(72 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto reserved_flags() const { return extract<std::uint32_t>(dynamic_offset(80 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto buffers_lost() const { return extract<std::uint32_t>(dynamic_offset(84 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto session_name() const { return extract_u16string(dynamic_offset(88 + 4 + time_zone_information_view::static_size, 2), session_name_string_length); }
    inline auto file_name() const { return extract_u16string(dynamic_offset(88 + 4 + time_zone_information_view::static_size + session_name().size() * 2 + 2, 2), log_file_name_string_length); }

    inline std::size_t dynamic_size() const { return dynamic_offset(88 + 4 + time_zone_information_view::static_size + session_name().size() * 2 + 2 + file_name().size() * 2 + 2, 2); }

private:
    mutable std::optional<std::size_t> session_name_string_length;
    mutable std::optional<std::size_t> log_file_name_string_length;
};

} // namespace snail::etl::parser
