
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"

namespace perfreader::etl::parser {

// See _SYSTEM_TRACE_HEADER in ntwmi.h
struct system_trace_header
{
    std::uint16_t version;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    wmi_trace_packet packet;

    std::uint32_t thread_id;
    std::uint32_t process_id;

    std::uint64_t system_time;
    std::uint32_t kernel_time;
    std::uint32_t user_time;
};

std::size_t parse(utility::binary_stream_parser& stream, system_trace_header& data);

} // namespace perfreader::etl::parser
