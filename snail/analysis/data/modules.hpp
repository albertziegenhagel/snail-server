#pragma once

#include <string>
#include <vector>

#include <snail/analysis/data/hit_counts.hpp>

namespace snail::analysis {

struct module_info
{
    using id_t = std::size_t;

    id_t id;

    std::string name;

    source_hit_counts hits;
};

} // namespace snail::analysis
