#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include <snail/data/types.hpp>

namespace snail::data {

struct hit_counts
{
    std::size_t total = 0;
    std::size_t self  = 0;
};

struct call_tree_node
{
    std::string name;

    hit_counts hits;

    std::unordered_map<instruction_pointer_t, std::size_t> self_hits;

    std::size_t number_of_children() const;

    const call_tree_node* top_total_hit_child() const;

    call_tree_node& find_or_add_child(std::string_view child_name);

private:
    std::vector<std::unique_ptr<call_tree_node>> children;
};

class call_tree
{
public:
    call_tree_node& root();
    const call_tree_node& root() const;

    void dump_hot_path() const;
private:
    call_tree_node root_node;
};

} // namespace snail::data
