
#pragma once

#include <cstdint>

#include <array>
#include <string>

#include "etl/parser/trace.hpp"
#include "etl/guid.hpp"

namespace perfreader::etl::parser {

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

} // namespace perfreader::etl::parser
