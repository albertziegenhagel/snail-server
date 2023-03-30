#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct thread_info
{
    common::process_id_t id;

    std::optional<std::string> name;

    // Time since session start
    std::chrono::nanoseconds start_time;
    std::chrono::nanoseconds end_time;
};

} // namespace snail::analysis
