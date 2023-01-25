#pragma once

#include <algorithm>
#include <cassert>
#include <ranges>
#include <unordered_map>
#include <vector>

#include <snail/common/hash_combine.hpp>

#include <snail/data/types.hpp>

namespace snail::analysis::detail {

class stack_cache
{
public:
    using stack_t = std::vector<data::instruction_pointer_t>;

    template<std::ranges::sized_range R>
    std::size_t insert(R&& stack_range);

    inline const stack_t& get(std::size_t stack_index) const;

private:
    std::vector<stack_t>                                      stacks;
    std::unordered_map<std::size_t, std::vector<std::size_t>> stack_map;

    struct stack_hasher
    {
        template<std::ranges::sized_range R>
        std::size_t operator()(R&& stack_range) const;

    private:
        std::hash<data::instruction_pointer_t> ip_hash;
    };
};

template<std::ranges::sized_range R>
std::size_t stack_cache::insert(R&& stack_range)
{
    const auto hash = stack_hasher()(stack_range);

    auto& saved_stack_indices = stack_map[hash];

    for(const auto& stack_index : saved_stack_indices)
    {
        const auto& saved_stack = stacks[stack_index];
        if(!std::ranges::equal(saved_stack, stack_range)) continue;

        return stack_index;
    }

    const auto new_stack_index = stacks.size();

    stacks.emplace_back();
    stacks.back().reserve(std::ranges::size(stack_range));
    std::ranges::copy(stack_range, std::back_inserter(stacks.back()));

    saved_stack_indices.push_back(new_stack_index);
    return new_stack_index;
}

inline const stack_cache::stack_t& stack_cache::get(std::size_t stack_index) const
{
    return stacks[stack_index];
}

template<std::ranges::sized_range R>
std::size_t stack_cache::stack_hasher::operator()(R&& stack_range) const
{
    std::size_t hash = std::ranges::size(stack_range);
    for(const auto ip : stack_range)
    {
        hash = common::hash_combine(hash, ip_hash(ip));
    }
    return hash;
}

} // namespace snail::analysis::detail
