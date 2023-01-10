
#pragma once

#include <cstdint>

#include <string>
#include <array>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::process

// See https://learn.microsoft.com/en-us/windows/win32/etw/process-typegroup1
struct event_process_v3_type_group1
{
    // 1=Load, 2=Unload, 3=DCStart, 4=DCEnd, 39=Defunct
    static inline constexpr std::array<std::uint8_t, 5> event_types = {1, 2, 3, 4, 39};
    static inline constexpr std::uint8_t event_version              = 3;

    pointer unique_process_key;
    std::uint32_t process_id;
    std::uint32_t parent_id;
    std::uint32_t session_id;
    std::int32_t exit_status;
    pointer directory_table_base;
    sid user_sid;
    std::string image_filename;
    std::u16string command_line;
};

std::size_t parse(utility::binary_stream_parser& stream, event_process_v3_type_group1& data);

} // namespace perfreader::etl::parser
