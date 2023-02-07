#pragma once

#include <type_traits>

namespace snail::analysis {

struct hit_counts
{
    std::size_t total = 0;
    std::size_t self  = 0;
};

} // namespace snail::analysis
