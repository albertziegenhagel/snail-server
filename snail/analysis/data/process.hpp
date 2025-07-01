#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <snail/analysis/data/ids.hpp>
#include <snail/analysis/data/pmc_counter.hpp>

namespace snail::analysis {

struct process_info
{
    unique_process_id unique_id;

    std::uint32_t os_id;

    std::string name;

    // Time since session start
    std::chrono::nanoseconds start_time;
    std::chrono::nanoseconds end_time;

    std::optional<std::size_t> context_switches;

    std::vector<pmc_counter_info> counters;
};

} // namespace snail::analysis
