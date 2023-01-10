
#include "etl/parser/trace_headers/instance_trace.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, instance_trace_header& data)
{
    auto read_bytes = parse(stream, data.size);
    read_bytes += parse(stream, data.header_type);
    assert(data.header_type == trace_header_type::instance32 ||
           data.header_type == trace_header_type::instance64);
    read_bytes += parse(stream, data.header_flags);
    read_bytes += parse(stream, data.version);
    read_bytes += parse(stream, data.thread_id);
    read_bytes += parse(stream, data.process_id);
    read_bytes += parse(stream, data.timestamp);
    read_bytes += parse(stream, data.guid);
    read_bytes += parse(stream, data.kernel_time);
    read_bytes += parse(stream, data.user_time);
    read_bytes += parse(stream, data.instance_id);
    read_bytes += parse(stream, data.parent_instance_id);
    read_bytes += parse(stream, data.parent_guid);
    return read_bytes;
}
