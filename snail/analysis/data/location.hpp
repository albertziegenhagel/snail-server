#pragma once

#include <string>

#include <snail/analysis/data/hit_counts.hpp>

namespace snail::analysis {

struct file_info
{
    using id_t = std::size_t;

    id_t id;

    std::string name;

    hit_counts hits;
};

struct location
{
    file_info::id_t file_id;

    std::size_t line_number;

    hit_counts hits;
};

} // namespace snail::analysis
