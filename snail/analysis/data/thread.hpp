#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <snail/analysis/data/ids.hpp>

namespace snail::analysis {

struct thread_info
{
    unique_thread_id unique_id;

    std::uint32_t os_id;

    std::optional<std::string> name;

    // Time since session start
    std::chrono::nanoseconds start_time;
    std::chrono::nanoseconds end_time;
};

} // namespace snail::analysis
