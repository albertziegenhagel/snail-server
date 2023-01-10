
#include "etl/parser/trace_headers/system_trace.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, system_trace_header& data)
{
    auto read_bytes = parse(stream, data.version);
    read_bytes += parse(stream, data.header_type);
    assert(data.header_type == trace_header_type::system32 ||
           data.header_type == trace_header_type::system64);
    read_bytes += parse(stream, data.header_flags);
    read_bytes += parse(stream, data.packet);
    read_bytes += parse(stream, data.thread_id);
    read_bytes += parse(stream, data.process_id);
    read_bytes += parse(stream, data.system_time);
    read_bytes += parse(stream, data.kernel_time);
    read_bytes += parse(stream, data.user_time);
    return read_bytes;
}