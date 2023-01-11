
#include "etl/parser/trace.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, wmi_trace_packet& data)
{
    auto read_bytes = parse(stream, data.size);
    read_bytes += parse(stream, data.type);
    read_bytes += parse(stream, data.group);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, generic_trace_marker& data)
{
    std::size_t read_bytes = parse(stream, data.reserved);
    read_bytes += parse(stream, data.header_type);
    read_bytes += parse(stream, data.header_flags);
    return read_bytes;
}

bool generic_trace_marker::is_trace_header() const
{
    return (header_flags & trace_header_flag) != 0;
}

bool generic_trace_marker::is_trace_header_event_trace() const
{
    return (header_flags & trace_header_event_trace_flag) != 0;
}

bool generic_trace_marker::is_trace_message() const
{
    return (header_flags & trace_message_flag) != 0;
}

bool generic_trace_marker_view::is_trace_header() const
{
    return (header_flags() & trace_header_flag) != 0;
}

bool generic_trace_marker_view::is_trace_header_event_trace() const
{
    return (header_flags() & trace_header_event_trace_flag) != 0;
}

bool generic_trace_marker_view::is_trace_message() const
{
    return (header_flags() & trace_message_flag) != 0;
}