#pragma once

#include <string>

#include <snail/analysis/data/hit_counts.hpp>

namespace snail::analysis {

struct file_info
{
    using id_t = std::size_t;

    id_t id;

    std::string path;

    hit_counts hits;
};

} // namespace snail::analysis
