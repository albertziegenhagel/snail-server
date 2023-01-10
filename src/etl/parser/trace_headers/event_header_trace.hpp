
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"
#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// See EVENT_HEADER_FLAG_* in evntcons.h
enum class event_header_flag : std::uint16_t
{
    extended_info   = 1,
    private_session = 2,
    string_only     = 4,
    trace_message   = 8,
    no_cputime      = 16,
    header_32_bit   = 32,
    header_64_bit   = 64,
    classic_header  = 256,
    processor_index = 512
};

// See EVENT_HEADER_PROPERTY_* in evntcons.h
enum class event_header_property_flag : std::uint16_t
{
    xml             = 1,
    forwarded_xml   = 2,
    legacy_eventlog = 4,
    reloggable      = 8
};

// See _EVENT_DESCRIPTOR in evntprov.h
struct event_descriptor
{
    std::uint16_t id;
    std::uint8_t version;
    std::uint8_t channel;
    std::uint8_t level;
    std::uint8_t opcode;
    std::uint16_t task;
    std::uint64_t keyword;
};

std::size_t parse(utility::binary_stream_parser& stream, event_descriptor& data);

// See EVENT_HEADER in evntcons.h
// the head structure (size, header_type, header_flags) has been adjusted
struct event_header_trace_header
{
    std::uint16_t size;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    std::underlying_type_t<event_header_flag> flags;
    std::underlying_type_t<event_header_property_flag> event_property;
    std::uint32_t thread_id;
    std::uint32_t process_id;
    std::uint64_t timestamp;
    parser::guid provider_id;
    parser::event_descriptor event_descriptor;
    std::uint64_t processor_time;
    parser::guid activity_id;
};

std::size_t parse(utility::binary_stream_parser& stream, event_header_trace_header& data);

} // namespace perfreader::etl::parser
