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

    std::optional<std::size_t> context_switches;

    struct counter_info
    {
        std::optional<std::string> name;
        std::size_t                count;
    };
    std::vector<counter_info> counters;
};

} // namespace snail::analysis
