
#include <snail/analysis/analysis.hpp>

#include <cassert>
#include <format>
#include <optional>

#include <snail/analysis/data_provider.hpp>

#include <snail/common/hash_combine.hpp>

using namespace snail;
using namespace snail::analysis;

namespace std {

template<typename T1, typename T2>
struct hash<std::pair<T1, T2>>
{
    [[nodiscard]] size_t operator()(const std::pair<T1, T2>& value) const noexcept
    {
        return common::hash_combine(first_hasher_(value.first), second_hasher_(value.second));
    }

private:
    std::hash<T1> first_hasher_;
    std::hash<T2> second_hasher_;
};

} // namespace std

namespace {

source_hit_counts init_hits(const sample_source_info::id_t max_source_id)
{
    return source_hit_counts{
        .counts_per_source = std::vector<hit_counts>(max_source_id + 1)};
}

module_info& get_or_create_module(std::vector<module_info>&                           modules,
                                  std::unordered_map<std::string, module_info::id_t>& modules_by_name,
                                  std::string_view                                    module_name,
                                  const sample_source_info::id_t                      max_source_id)
{
    const auto key = std::string(module_name);

    auto iter = modules_by_name.find(key);
    if(iter == modules_by_name.end())
    {
        modules.push_back(module_info{
            .id   = modules.size(),
            .name = std::string(module_name),
            .hits = init_hits(max_source_id),
        });
        modules_by_name[key] = modules.back().id;
        return modules.back();
    }

    return modules[iter->second];
}

function_info& get_or_create_function(std::vector<function_info>&                                                       functions,
                                      std::unordered_map<std::pair<module_info::id_t, std::string>, module_info::id_t>& functions_by_name,
                                      const module_info&                                                                module,
                                      std::string_view                                                                  function_name,
                                      const sample_source_info::id_t                                                    max_source_id)
{
    const auto key = std::make_pair(module.id, std::string(function_name));

    auto iter = functions_by_name.find(key);
    if(iter == functions_by_name.end())
    {
        functions.push_back(function_info{
            .id           = functions.size(),
            .module_id    = module.id,
            .name         = std::string(function_name),
            .hits         = init_hits(max_source_id),
            .callers      = {},
            .callees      = {},
            .file_id      = {},
            .line_number  = {},
            .hits_by_line = {},
        });
        functions_by_name[key] = functions.back().id;
        return functions.back();
    }

    return functions[iter->second];
}

file_info& get_or_create_file(std::vector<file_info>&                             files,
                              std::unordered_map<std::string, module_info::id_t>& files_by_path,
                              std::string_view                                    file_path,
                              const sample_source_info::id_t                      max_source_id)
{
    const auto key = std::string(file_path);

    auto iter = files_by_path.find(key);
    if(iter == files_by_path.end())
    {
        files.push_back(file_info{
            .id   = files.size(),
            .path = std::string(file_path),
            .hits = init_hits(max_source_id),
        });
        files_by_path[key] = files.back().id;
        return files.back();
    }

    return files[iter->second];
}

call_tree_node& get_or_append_call_tree_child(std::vector<call_tree_node>&   call_tree_nodes,
                                              call_tree_node&                current_node,
                                              const function_info&           function,
                                              const sample_source_info::id_t max_source_id)
{
    for(const auto child_id : current_node.children)
    {
        auto& child_node = call_tree_nodes[child_id];
        if(child_node.function_id == function.id)
        {
            return child_node;
        }
    }

    const auto new_node_id = call_tree_nodes.size();
    current_node.children.push_back(new_node_id);

    // ATTENTION: changing `call_tree_nodes` might invalidate `current_node`.
    call_tree_nodes.push_back(call_tree_node{
        .id          = new_node_id,
        .function_id = function.id,
        .hits        = init_hits(max_source_id),
        .children    = {},
    });
    return call_tree_nodes.back();
}

} // namespace

const module_info& stacks_analysis::get_module(module_info::id_t id) const
{
    return modules.at(id);
}

const function_info& stacks_analysis::get_function_root() const
{
    return function_root;
}

const function_info& stacks_analysis::get_function(function_info::id_t id) const
{
    return id == function_root.id ? function_root : functions.at(id);
}

const std::vector<function_info>& stacks_analysis::all_functions() const
{
    return functions;
}

const std::vector<module_info>& stacks_analysis::all_modules() const
{
    return modules;
}

const std::vector<file_info>& stacks_analysis::all_files() const
{
    return files;
}

const file_info& stacks_analysis::get_file(file_info::id_t id) const
{
    return files.at(id);
}

const call_tree_node& stacks_analysis::get_call_tree_root() const
{
    return call_tree_root;
}

const call_tree_node& stacks_analysis::get_call_tree_node(call_tree_node::id_t id) const
{
    return id == call_tree_root.id ? call_tree_root : call_tree_nodes.at(id);
}

stacks_analysis snail::analysis::analyze_stacks(const samples_provider& provider,
                                                unique_process_id       process_id,
                                                const sample_filter&    filter)
{
    std::unordered_map<std::string, module_info::id_t>                               modules_by_name;
    std::unordered_map<std::pair<module_info::id_t, std::string>, module_info::id_t> functions_by_name;
    std::unordered_map<std::string, file_info::id_t>                                 files_by_path;

    stacks_analysis result;
    result.process_id = process_id;

    const auto sources       = provider.sample_sources();
    const auto max_source_id = sources.back().id;

    result.function_root = function_info{
        .id           = stacks_analysis::root_function_id,
        .module_id    = std::size_t(-1),
        .name         = "root",
        .hits         = init_hits(max_source_id),
        .callers      = {},
        .callees      = {},
        .file_id      = {},
        .line_number  = {},
        .hits_by_line = {},
    };

    result.call_tree_root = call_tree_node{
        .id          = stacks_analysis::root_call_tree_node_id,
        .function_id = result.function_root.id,
        .hits        = init_hits(max_source_id),
        .children    = {},
    };

    for(const auto& source_info : provider.sample_sources())
    {
        for(const auto& sample : provider.samples(source_info.id, process_id, filter))
        {
            if(sample.has_stack())
            {
                ++result.call_tree_root.hits.get(source_info.id).total;
                ++result.function_root.hits.get(source_info.id).total;

                std::optional<module_info::id_t>   previous_node_id;
                std::optional<module_info::id_t>   previous_module_id;
                std::optional<function_info::id_t> previous_function_id;
                std::optional<file_info::id_t>     previous_file_id;
                std::optional<std::size_t>         previous_line_number;

                for(const auto stack_frame : sample.reversed_stack())
                {
                    auto&       module   = get_or_create_module(result.modules, modules_by_name, stack_frame.module_name, max_source_id);
                    auto&       function = get_or_create_function(result.functions, functions_by_name, module, stack_frame.symbol_name, max_source_id);
                    auto&       node     = get_or_append_call_tree_child(result.call_tree_nodes, previous_node_id ? result.call_tree_nodes[*previous_node_id] : result.call_tree_root, function, max_source_id);
                    auto* const file     = stack_frame.file_path.empty() ? nullptr : &get_or_create_file(result.files, files_by_path, stack_frame.file_path, max_source_id);

                    ++module.hits.get(source_info.id).total;
                    ++function.hits.get(source_info.id).total;
                    ++node.hits.get(source_info.id).total;
                    if(file != nullptr) ++file->hits.get(source_info.id).total;

                    if(previous_function_id)
                    {
                        auto& previous_function = result.functions[*previous_function_id];
                        ++previous_function.callees[function.id].get(source_info.id).total;
                        ++function.callers[previous_function.id].get(source_info.id).total;
                    }
                    else
                    {
                        auto& previous_function = result.function_root;
                        ++previous_function.callees[function.id].get(source_info.id).total;
                        ++function.callers[previous_function.id].get(source_info.id).total;
                    }

                    if(file != nullptr)
                    {
                        assert(function.file_id == std::nullopt || *function.file_id == file->id);
                        if(function.file_id == std::nullopt)
                        {
                            function.file_id = file->id;
                        }
                        // assert(function.line_number == std::nullopt || *function.line_number == stack_frame.function_line_number);
                        if(function.line_number == std::nullopt)
                        {
                            function.line_number = stack_frame.function_line_number;
                        }
                        ++function.hits_by_line[stack_frame.instruction_line_number].get(source_info.id).total;
                    }

                    previous_module_id   = module.id;
                    previous_function_id = function.id;
                    previous_node_id     = node.id;
                    previous_file_id     = file == nullptr ? std::nullopt : std::optional(file->id);
                    previous_line_number = stack_frame.instruction_line_number;
                }

                // final elements at the top of the stack have self hits
                if(previous_module_id) ++result.modules[*previous_module_id].hits.get(source_info.id).self;
                if(previous_node_id) ++result.call_tree_nodes[*previous_node_id].hits.get(source_info.id).self;
                else ++result.call_tree_root.hits.get(source_info.id).self;
                if(previous_file_id) ++result.files[*previous_file_id].hits.get(source_info.id).self;
                if(previous_function_id)
                {
                    ++result.functions[*previous_function_id].hits.get(source_info.id).self;
                    if(previous_file_id && previous_line_number)
                    {
                        assert(*previous_file_id == result.functions[*previous_function_id].file_id);
                        ++result.functions[*previous_function_id].hits_by_line[*previous_line_number].get(source_info.id).self;
                    }
                }
                else
                {
                    ++result.function_root.hits.get(source_info.id).self;
                }
            }
            else if(sample.has_frame())
            {
                const auto stack_frame = sample.frame();

                auto&       module   = get_or_create_module(result.modules, modules_by_name, stack_frame.module_name, max_source_id);
                auto&       function = get_or_create_function(result.functions, functions_by_name, module, stack_frame.symbol_name, max_source_id);
                auto* const file     = stack_frame.file_path.empty() ? nullptr : &get_or_create_file(result.files, files_by_path, stack_frame.file_path, max_source_id);

                ++module.hits.get(source_info.id).total;
                ++function.hits.get(source_info.id).total;
                if(file != nullptr) ++file->hits.get(source_info.id).total;

                ++module.hits.get(source_info.id).self;
                ++function.hits.get(source_info.id).self;
                if(file != nullptr) ++file->hits.get(source_info.id).self;

                if(file != nullptr)
                {
                    assert(function.file_id == std::nullopt || *function.file_id == file->id);
                    if(function.file_id == std::nullopt)
                    {
                        function.file_id = file->id;
                    }
                    assert(function.line_number == std::nullopt || *function.line_number == stack_frame.function_line_number);
                    if(function.line_number == std::nullopt)
                    {
                        function.line_number = stack_frame.function_line_number;
                    }
                    ++function.hits_by_line[stack_frame.instruction_line_number].get(source_info.id).total;
                    ++function.hits_by_line[stack_frame.instruction_line_number].get(source_info.id).self;
                }
            }
        }
    }

    return result;
}
