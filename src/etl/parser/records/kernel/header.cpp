
#include "etl/parser/records/kernel/header.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(utility::binary_stream_parser& stream, event_trace_header_v2& data)
{
    auto read_bytes = parse(stream, data.buffer_size);
    read_bytes += parse(stream, data.version);
    read_bytes += parse(stream, data.provider_version);
    read_bytes += parse(stream, data.number_of_processors);
    read_bytes += parse(stream, data.end_time);
    read_bytes += parse(stream, data.timer_resolution);
    read_bytes += parse(stream, data.max_file_size);
    read_bytes += parse(stream, data.log_file_mode);
    read_bytes += parse(stream, data.buffers_written);
    read_bytes += parse(stream, data.start_buffers);
    read_bytes += parse(stream, data.pointer_size);
    read_bytes += parse(stream, data.events_lost);
    read_bytes += parse(stream, data.cpu_speed);
    read_bytes += parse(stream, data.logger_name, data.pointer_size);
    read_bytes += parse(stream, data.log_file_name, data.pointer_size);
    read_bytes += parse(stream, data.time_zone_information_);
    read_bytes += parse(stream, data.boot_time);
    read_bytes += parse(stream, data.perf_freq);
    read_bytes += parse(stream, data.start_time);
    read_bytes += parse(stream, data.reserved_flags);
    read_bytes += parse(stream, data.buffers_lost);
    read_bytes += stream.read(data.session_name_string);
    read_bytes += stream.read(data.log_file_name_string);
    // std::u16string session_name_string;
    // std::u16string log_file_name_string;
    return read_bytes;
}