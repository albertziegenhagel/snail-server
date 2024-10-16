#pragma once

#include <limits>

#include <snail/common/progress.hpp>

#include <snail/analysis/data/call_tree.hpp>
#include <snail/analysis/data/file.hpp>
#include <snail/analysis/data/functions.hpp>
#include <snail/analysis/data/ids.hpp>
#include <snail/analysis/data/modules.hpp>
#include <snail/analysis/data/process.hpp>
#include <snail/analysis/data/sample_source.hpp>

#include <snail/analysis/options.hpp>

namespace snail::analysis {

class samples_provider;

struct stacks_analysis;

stacks_analysis analyze_stacks(const samples_provider&           provider,
                               unique_process_id                 process_id,
                               const sample_filter&              filter             = {},
                               const common::progress_listener*  progress_listener  = nullptr,
                               const common::cancellation_token* cancellation_token = nullptr);

struct stacks_analysis
{
    unique_process_id process_id;

    const module_info& get_module(module_info::id_t id) const;

    const function_info& get_function_root() const;
    const function_info& get_function(function_info::id_t id) const;

    const call_tree_node& get_call_tree_root() const;
    const call_tree_node& get_call_tree_node(call_tree_node::id_t id) const;

    const file_info& get_file(file_info::id_t id) const;

    const std::vector<module_info>&   all_modules() const;
    const std::vector<function_info>& all_functions() const;
    const std::vector<file_info>&     all_files() const;

private:
    friend stacks_analysis analyze_stacks(const samples_provider&           provider,
                                          unique_process_id                 process_id,
                                          const sample_filter&              filter,
                                          const common::progress_listener*  progress_listener,
                                          const common::cancellation_token* cancellation_token);

    // FIXME: This is a workaround: we would like to use the max of std::size_t, but since we will
    //        serialize those values to JSON and eventually handle them in JavaScript which cannot
    //        deal with integers larger than 53bits in its `number` type, we restrict ourselves
    //        to 32bits here.
    static_assert(sizeof(std::size_t) >= sizeof(std::uint32_t)); // just for sanity. Otherwise this would not be a "restriction".
    constexpr static inline auto root_function_id       = std::numeric_limits<std::uint32_t>::max();
    constexpr static inline auto root_call_tree_node_id = std::numeric_limits<std::uint32_t>::max();

    std::vector<module_info>    modules;
    std::vector<function_info>  functions;
    std::vector<call_tree_node> call_tree_nodes;
    std::vector<file_info>      files;

    call_tree_node call_tree_root;
    function_info  function_root;
};

} // namespace snail::analysis
