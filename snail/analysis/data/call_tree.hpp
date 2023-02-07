#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <snail/analysis/data/functions.hpp>
#include <snail/analysis/data/hit_counts.hpp>

namespace snail::analysis {

struct call_tree_node
{
    using id_t = std::size_t;

    id_t id;

    function_info::id_t function_id;

    hit_counts hits;

    std::vector<id_t> children;
};

} // namespace snail::analysis
