
#include <snail/analysis/call_tree.hpp>

#include <format>

#include <snail/analysis/stack_provider.hpp>

using namespace snail;
using namespace snail::analysis;

data::call_tree snail::analysis::build_call_tree(const analysis::stack_provider& stack_provider,
                                                 const process_info& process)
{
    data::call_tree result;
    auto& root_node = result.root();

    root_node.name = std::format("{} (PID: {})", process.image_name(), process.process_id());

    for(const auto& sample : stack_provider.samples(process))
    {
        ++root_node.hits.total;

        // missing stack data is assigned as self hit to the process
        if(!sample.has_stack())
        {
            ++root_node.hits.self;
            continue;
        }

        auto* current_node = &root_node;
        for(const auto& stack_entry : sample.reversed_stack())
        {
            current_node = &current_node->find_or_add_child(stack_entry.symbol_name());

            ++current_node->hits.total;
        }
        ++current_node->hits.self;
    }

    return result;
}
