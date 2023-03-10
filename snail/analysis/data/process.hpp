#pragma once

#include <string>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct process_info
{
    common::process_id_t id;

    std::string name;

    common::timestamp_t start_time;
    common::timestamp_t end_time;
};

} // namespace snail::analysis
