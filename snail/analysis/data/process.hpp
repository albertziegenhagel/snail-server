#pragma once

#include <string>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct process_info
{
    common::process_id_t process_id;

    std::string name;
};

} // namespace snail::analysis
