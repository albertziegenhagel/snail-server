
#include "etl/parser/records/kernel/image.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(utility::binary_stream_parser& stream, event_image_v2_load& data, std::uint32_t pointer_size)
{
    auto read_bytes = parse(stream, data.image_base, pointer_size);
    read_bytes += parse(stream, data.image_size, pointer_size);
    read_bytes += parse(stream, data.process_id);
    read_bytes += parse(stream, data.image_checksum);
    read_bytes += parse(stream, data.time_date_stamp);
    read_bytes += parse(stream, data.reserved_0);
    read_bytes += parse(stream, data.default_base, pointer_size);
    read_bytes += parse(stream, data.reserved_1);
    read_bytes += parse(stream, data.reserved_2);
    read_bytes += parse(stream, data.reserved_3);
    read_bytes += parse(stream, data.reserved_4);
    read_bytes += stream.read(data.file_name);
    return read_bytes;
}