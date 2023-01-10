
#include "etl/parser/buffer.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, etw_buffer_context& data)
{
    auto read_bytes = parse(stream, data.processor_index);
    read_bytes += parse(stream, data.logger_id);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, wnode_header& data)
{
    auto read_bytes = parse(stream, data.buffer_size);
    read_bytes += parse(stream, data.saved_offset);
    read_bytes += parse(stream, data.current_offset);
    read_bytes += parse(stream, data.reference_count);
    read_bytes += parse(stream, data.timestamp);
    read_bytes += parse(stream, data.sequence_number);
    read_bytes += parse(stream, data.clock);
    read_bytes += parse(stream, data.client_context);
    read_bytes += parse(stream, data.state);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, wmi_buffer_header& data)
{
    auto read_bytes = parse(stream, data.wnode);
    read_bytes += parse(stream, data.offset);
    read_bytes += parse(stream, data.buffer_flag);
    read_bytes += parse(stream, data.buffer_type);
    read_bytes += parse(stream, data.reference_time.start_time);
    read_bytes += parse(stream, data.reference_time.start_perf_clock);
    return read_bytes;
}
