
#pragma once

#include <cstdint>

#include <array>
#include <string>

#include <snail/etl/parser/trace.hpp>
#include <snail/etl/guid.hpp>

namespace snail::etl::parser {

struct event_identifier_group
{
    event_trace_group group;
    std::uint8_t      type;
    std::string_view  name;
};

struct event_identifier_guid
{
    etl::guid         guid;
    std::uint8_t      type;
    std::string_view  name;
};

} // namespace snail::etl::parser
