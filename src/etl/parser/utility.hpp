
#pragma once

#include <cstdint>
#include <cstddef>

#include "binarystream/fwd/binarystream.hpp"

namespace perfreader::etl::parser {

struct pointer
{
    std::uint64_t address; // always store 64-bit addresses
};

std::size_t parse(utility::binary_stream_parser& stream, pointer& data, std::uint32_t pointer_size);

// See SYSTEMTIME from minwinbase.h
struct system_time
{
    std::int16_t year;
    std::int16_t month;
    std::int16_t dayofweek;
    std::int16_t day;
    std::int16_t hour;
    std::int16_t minute;
    std::int16_t second;
    std::int16_t milliseconds;
};

std::size_t parse(utility::binary_stream_parser& stream, system_time& data);

// See TIME_ZONE_INFORMATION from timezoneapi.h
struct time_zone_information
{
    static_assert(sizeof(char16_t) == 2);

    std::uint32_t bias;
    char16_t      standard_name[32];
    system_time   standard_date;
    std::uint32_t standard_bias;
    char16_t      daylight_name[32];
    system_time   daylight_date;
    std::uint32_t daylight_bias;
};

std::size_t parse(utility::binary_stream_parser& stream, time_zone_information& data);

// See GUID from guiddef.h
struct guid
{
    std::uint32_t data_1;
    std::uint16_t data_2;
    std::uint16_t data_3;
    std::uint8_t  data_4[8];
};

std::size_t parse(utility::binary_stream_parser& stream, guid& data);

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/f992ad60-0fe4-4b87-9fed-beb478836861
struct sid
{
    std::uint8_t revision;
    std::uint8_t sub_authority_count;
    std::uint8_t identifier_authority[6];
    std::uint32_t sub_authority[16]; // 16 is max length. actuall length given by sub_authority_count
};

std::size_t parse(utility::binary_stream_parser& stream, sid& data);

} // namespace perfreader::etl::parser
