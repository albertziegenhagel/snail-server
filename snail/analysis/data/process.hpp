#pragma once

#include <chrono>
#include <string>

#include <snail/analysis/data/ids.hpp>

namespace snail::analysis {

struct process_info
{
    unique_process_id unique_id;

    std::uint32_t os_id;

    std::string name;

    // Time since session start
    std::chrono::nanoseconds start_time;
    std::chrono::nanoseconds end_time;
};

} // namespace snail::analysis
