#pragma once

#include <snail/common/types.hpp>

#include <snail/analysis/data/call_tree.hpp>
#include <snail/analysis/data/functions.hpp>
#include <snail/analysis/data/modules.hpp>
#include <snail/analysis/data/process.hpp>

namespace snail::analysis {

class stack_provider;

struct stacks_analysis
{
    process_info process;

    std::vector<module_info>    modules;
    std::vector<function_info>  functions;
    std::vector<call_tree_node> call_tree_nodes;

    call_tree_node call_tree_root;
    function_info  function_root;
};

stacks_analysis analyze_stacks(const analysis::stack_provider& stack_provider,
                               common::process_id_t            process_id);

} // namespace snail::analysis