
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"
#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

struct trace_class
{
    std::uint8_t type;
    std::uint8_t level;
    std::uint16_t version;
};

std::size_t parse(utility::binary_stream_parser& stream, trace_class& data);

// See EVENT_TRACE_HEADER in evntrace.h
struct full_header_trace_header
{
    std::uint16_t size;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    trace_class class_;

    std::uint32_t thread_id;
    std::uint32_t process_id;

    std::uint64_t timestamp;
    parser::guid guid;
    std::uint64_t processor_time;
};

std::size_t parse(utility::binary_stream_parser& stream, full_header_trace_header& data);

} // namespace perfreader::etl::parser
