#pragma once

#include <type_traits>

namespace snail::analysis {

struct hit_counts
{
    std::size_t total = 0;
    std::size_t self  = 0;

    friend bool operator==(const hit_counts&, const hit_counts&) = default;
};

} // namespace snail::analysis
