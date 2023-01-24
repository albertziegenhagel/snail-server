#pragma once

#include <snail/data/call_tree.hpp>
#include <snail/data/types.hpp>

namespace snail::analysis {

class stack_provider;
struct process_info;

data::call_tree build_call_tree(const analysis::stack_provider& stack_provider,
                                const process_info& process);

} // namespace snail::analysis