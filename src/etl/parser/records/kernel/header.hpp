
#pragma once

#include <cstdint>

#include <string>
#include <array>
#include <optional>

#include "etl/parser/extract.hpp"
#include "etl/parser/utility.hpp"

//
// event records for event_trace_group::header
//

namespace perfreader::etl::parser {

// `EventTrace_Header:EventTraceEvent` from wmicore.mof in WDK 10.0.22621.0
struct event_trace_v2_header_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        header = 0,
    };
    static inline constexpr std::array<std::uint8_t,  1> event_types    = {0};
    static inline constexpr std::array<std::uint16_t, 1> event_versions = {2};

    inline explicit event_trace_v2_header_event_view(std::span<const std::byte> buffer) :
        extract_view_dynamic_base(buffer, parser::extract<std::uint32_t>(buffer, 44))
    {}

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto buffer_size() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto version() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }
    inline auto provider_version() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); }
    inline auto number_of_processors() const { return extract<std::uint32_t>(dynamic_offset(12, 0)); }
    inline auto end_time() const { return extract<std::uint64_t>(dynamic_offset(16, 0)); }
    inline auto timer_resolution() const { return extract<std::uint32_t>(dynamic_offset(24, 0)); }
    inline auto max_file_size() const { return extract<std::uint32_t>(dynamic_offset(28, 0)); }
    inline auto log_file_mode() const { return extract<std::uint32_t>(dynamic_offset(32, 0)); }
    inline auto buffers_written() const { return extract<std::uint32_t>(dynamic_offset(36, 0)); }
    inline auto start_buffers() const { return extract<std::uint32_t>(dynamic_offset(40, 0)); }
    inline auto pointer_size() const { return extract<std::uint32_t>(dynamic_offset(44, 0)); }
    inline auto events_lost() const { return extract<std::uint32_t>(dynamic_offset(48, 0)); }
    inline auto cpu_speed() const { return extract<std::uint32_t>(dynamic_offset(52, 0)); }
    inline auto logger_name() const { return extract_pointer(dynamic_offset(56, 0)); }
    inline auto log_file_name() const { return extract_pointer(dynamic_offset(56, 1)); }
    inline auto time_zone_information() const { return time_zone_information_view(buffer_.subspan(dynamic_offset(56, 2))); }
    // NOTE: for some reason the size of `time_zone_information` is given as 176 bytes in wmicore.mof
    //       but our data structure is only 172 bytes long. Hence we add an additional 4 bytes to the
    //       offset in the following. 
    inline auto boot_time() const { return extract<std::uint64_t>(dynamic_offset(56 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto perf_freq() const { return extract<std::uint64_t>(dynamic_offset(64 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto start_time() const { return extract<std::uint64_t>(dynamic_offset(72 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto reserved_flags() const { return extract<std::uint32_t>(dynamic_offset(80 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto buffers_lost() const { return extract<std::uint32_t>(dynamic_offset(84 + 4 + time_zone_information_view::static_size, 2)); }
    inline auto session_name() const { return extract_u16string(dynamic_offset(88 + 4 + time_zone_information_view::static_size, 2), session_name_string_length); }
    inline auto file_name() const { return extract_u16string(dynamic_offset(88 + 4 + session_name().size()+2, 2), log_file_name_string_length); }

private:
    mutable std::optional<std::size_t> session_name_string_length;
    mutable std::optional<std::size_t> log_file_name_string_length;
};

} // namespace perfreader::etl::parser
