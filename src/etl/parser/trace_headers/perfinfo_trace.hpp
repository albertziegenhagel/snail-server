
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"

namespace perfreader::etl::parser {

// See _PERFINFO_TRACE_HEADER in ntwmi.h
struct perfinfo_trace_header
{
    std::uint16_t version;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    wmi_trace_packet packet;

    std::uint64_t system_time;
};

std::size_t parse(utility::binary_stream_parser& stream, perfinfo_trace_header& data);

} // namespace perfreader::etl::parser
