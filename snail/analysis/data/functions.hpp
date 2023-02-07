#pragma once

#include <string>
#include <unordered_map>

#include <snail/analysis/data/hit_counts.hpp>
#include <snail/analysis/data/modules.hpp>

namespace snail::analysis {

struct function_info
{
    using id_t = std::size_t;

    id_t id;

    module_info::id_t module_id;

    std::string name;

    hit_counts hits;

    std::unordered_map<function_info::id_t, hit_counts> callers;
    std::unordered_map<function_info::id_t, hit_counts> callees;
};

} // namespace snail::analysis
