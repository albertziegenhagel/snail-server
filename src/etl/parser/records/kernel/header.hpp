
#pragma once

#include <cstdint>

#include <type_traits>
#include <string>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

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

} // namespace perfreader::etl::parser
