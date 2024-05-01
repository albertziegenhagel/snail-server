#pragma once

#include <cassert>
#include <type_traits>
#include <vector>

#include <snail/analysis/data/sample_source.hpp>

namespace snail::analysis {

struct hit_counts
{
    std::size_t total = 0;
    std::size_t self  = 0;

    friend bool operator==(const hit_counts&, const hit_counts&) = default;
};

struct source_hit_counts
{
    hit_counts& get(sample_source_info::id_t source_id)
    {
        if(source_id >= counts_per_source.size()) counts_per_source.resize(source_id + 1);
        return counts_per_source[source_id];
    }
    const hit_counts& get(sample_source_info::id_t source_id) const
    {
        static constexpr hit_counts empty_hits = {};
        if(source_id >= counts_per_source.size()) return empty_hits;
        return counts_per_source[source_id];
    }

    friend bool operator==(const source_hit_counts&, const source_hit_counts&) = default;

    std::vector<hit_counts> counts_per_source;
};

} // namespace snail::analysis
