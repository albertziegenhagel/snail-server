
#pragma once

#include <cstdint>

#include <array>
#include <string>

#include <snail/common/guid.hpp>

#include <snail/etl/parser/trace.hpp>

namespace snail::etl::parser {

struct event_identifier_group
{
    event_trace_group group;
    std::uint8_t      type;
    std::string_view  name;
};

struct event_identifier_guid
{
    common::guid     guid;
    std::uint8_t     type;
    std::string_view name;
};

} // namespace snail::etl::parser
