
#pragma once

#include <cstdint>

#include <string>
#include <array>
#include <optional>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"
#include "etl/parser/extract.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::header

struct event_trace_header_v2
{
    static inline constexpr std::uint8_t event_type    = 0;
    static inline constexpr std::uint8_t event_version = 2;

    std::uint32_t buffer_size;
    union
    {
        std::uint32_t version;
        struct
        {
            std::uint8_t major;
            std::uint8_t minor;
            std::uint8_t sub;
            std::uint8_t sub_minor;
        } version_detail;
    };
    std::uint32_t provider_version;
    std::uint32_t number_of_processors;
    std::uint64_t end_time;
    std::uint32_t timer_resolution;
    std::uint32_t max_file_size;
    std::uint32_t log_file_mode;
    std::uint32_t buffers_written;
    std::uint32_t start_buffers;
    std::uint32_t pointer_size;
    std::uint32_t events_lost;
    std::uint32_t cpu_speed;
    pointer logger_name;
    pointer log_file_name;
    time_zone_information time_zone_information_;
    std::uint64_t boot_time;
    std::uint64_t perf_freq;
    std::uint64_t start_time;
    std::uint32_t reserved_flags;
    std::uint32_t buffers_lost;
    std::u16string session_name_string;
    std::u16string log_file_name_string;
};

std::size_t parse(utility::binary_stream_parser& stream, event_trace_header_v2& data);

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
