
#include "etl/parser/trace_headers/event_header_trace.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, event_descriptor& data)
{
    auto read_bytes = parse(stream, data.id);
    read_bytes += parse(stream, data.version);
    read_bytes += parse(stream, data.channel);
    read_bytes += parse(stream, data.level);
    read_bytes += parse(stream, data.opcode);
    read_bytes += parse(stream, data.task);
    read_bytes += parse(stream, data.keyword);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, event_header_trace_header& data)
{
    auto read_bytes = parse(stream, data.size);
    read_bytes += parse(stream, data.header_type);
    assert(data.header_type == trace_header_type::event_header32 ||
           data.header_type == trace_header_type::event_header64);
    read_bytes += parse(stream, data.header_flags);
    read_bytes += parse(stream, data.flags);
    read_bytes += parse(stream, data.event_property);
    read_bytes += parse(stream, data.thread_id);
    read_bytes += parse(stream, data.process_id);
    read_bytes += parse(stream, data.timestamp);
    read_bytes += parse(stream, data.provider_id);
    read_bytes += parse(stream, data.event_descriptor);
    read_bytes += parse(stream, data.processor_time);
    read_bytes += parse(stream, data.activity_id);
    return read_bytes;
}
