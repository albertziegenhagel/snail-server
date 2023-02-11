#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <snail/analysis/data/file.hpp>
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

    std::optional<file_info::id_t>              file_id;
    std::optional<std::size_t>                  line_number;
    std::unordered_map<std::size_t, hit_counts> hits_by_line;
};

} // namespace snail::analysis
