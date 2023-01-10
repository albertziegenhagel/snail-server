
#include "etl/parser/trace_headers/full_header_trace.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, trace_class& data)
{
    auto read_bytes = parse(stream, data.type);
    read_bytes += parse(stream, data.level);
    read_bytes += parse(stream, data.version);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, full_header_trace_header& data)
{
    auto read_bytes = parse(stream, data.size);
    read_bytes += parse(stream, data.header_type);
    assert(data.header_type == trace_header_type::full_header32 ||
           data.header_type == trace_header_type::full_header64);
    read_bytes += parse(stream, data.header_flags);
    read_bytes += parse(stream, data.class_);
    read_bytes += parse(stream, data.thread_id);
    read_bytes += parse(stream, data.process_id);
    read_bytes += parse(stream, data.timestamp);
    read_bytes += parse(stream, data.guid);
    read_bytes += parse(stream, data.processor_time);
    return read_bytes;
}
