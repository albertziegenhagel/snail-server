
#include <snail/data/call_tree.hpp>

#include <iostream>
#include <format>

using namespace snail;
using namespace snail::data;

call_tree_node& call_tree::root()
{
    return root_node;
}

const call_tree_node& call_tree::root() const
{
    return root_node;
}

void call_tree::dump_hot_path() const
{
    const auto* current_node = &root_node;

    while(true)
    {
        std::cout << std::format("{:140} {} {}", current_node->name, current_node->hits.total, current_node->hits.self) << "\n";

        const auto* const top_child = current_node->top_total_hit_child();

        if(top_child == nullptr) break;

        if(current_node->hits.self > top_child->hits.total) break;

        current_node = top_child;
    }
}

std::size_t call_tree_node::number_of_children() const
{
    return children.size();
}

const call_tree_node* call_tree_node::top_total_hit_child() const
{
    if(children.empty()) return nullptr;
    
    const auto* top_child = children.front().get();
    for(std::size_t i = 1; i < children.size(); ++i)
    {
        if(children[i]->hits.total > top_child->hits.total)
        {
            top_child = children[i].get();
        }
    }
    return top_child;
}

call_tree_node& call_tree_node::find_or_add_child(std::string_view child_name)
{
    for(const auto& child : children)
    {
        if(child->name == child_name) return *child.get();
    }

    auto new_child = std::make_unique<call_tree_node>();
    new_child->name = child_name;
    children.push_back(std::move(new_child));
    return *children.back();
}
