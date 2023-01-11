
#include "etl/parser/records/kernel/perfinfo.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(utility::binary_stream_parser& stream, event_perfinfo_v2_sampled_profile& data, std::uint32_t pointer_size)
{
    auto read_bytes = parse(stream, data.instruction_pointer, pointer_size);
    read_bytes += parse(stream, data.thread_id);
    read_bytes += parse(stream, data.count);
    return read_bytes;
}
