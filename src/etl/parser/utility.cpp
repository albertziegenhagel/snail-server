
#include "etl/parser/utility.hpp"

#include "etl/parser/generic.hpp"

using namespace perfreader::utility;
using namespace perfreader::etl::parser;

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, pointer& data, std::uint32_t pointer_size)
{
    assert(pointer_size == 4 || pointer_size == 8);

    if(pointer_size == 4)
    {
        std::uint32_t address32bit;
        const auto read_bytes = parse(stream, address32bit);
        data.address = address32bit;
        return read_bytes;
    }
    
    assert(pointer_size == 8);
    return parse(stream, data.address);
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, system_time& data)
{
    auto read_bytes = parse(stream, data.year);
    read_bytes += parse(stream, data.month);
    read_bytes += parse(stream, data.dayofweek);
    read_bytes += parse(stream, data.day);
    read_bytes += parse(stream, data.hour);
    read_bytes += parse(stream, data.minute);
    read_bytes += parse(stream, data.second);
    read_bytes += parse(stream, data.milliseconds);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, time_zone_information& data)
{
    auto read_bytes = parse(stream, data.bias);
    read_bytes += parse(stream, data.standard_name);
    read_bytes += parse(stream, data.standard_date);
    read_bytes += parse(stream, data.standard_bias);
    read_bytes += parse(stream, data.daylight_name);
    read_bytes += parse(stream, data.daylight_date);
    read_bytes += parse(stream, data.daylight_bias);
    return read_bytes;
}

std::size_t perfreader::etl::parser::parse(binary_stream_parser& stream, guid& data)
{
    auto read_bytes = parse(stream, data.data_1);
    read_bytes += parse(stream, data.data_2);
    read_bytes += parse(stream, data.data_3);
    read_bytes += parse(stream, data.data_4);
    return read_bytes;
}
