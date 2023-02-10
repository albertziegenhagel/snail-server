#pragma once

#include <optional>
#include <string>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct thread_info
{
    common::process_id_t id;

    std::optional<std::string> name;

    common::timestamp_t start_time;
    common::timestamp_t end_time;
};

} // namespace snail::analysis
