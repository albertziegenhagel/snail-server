#pragma once

#include <cstdint>

#include <chrono>
#include <string>

namespace snail::analysis {

struct session_info
{
    std::string command_line;

    std::chrono::sys_seconds date;
    std::chrono::nanoseconds runtime;

    std::size_t number_of_processes;
    std::size_t number_of_threads;

    std::size_t number_of_samples;
};

} // namespace snail::analysis
