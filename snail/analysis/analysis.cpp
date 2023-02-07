
#include <snail/analysis/analysis.hpp>

#include <format>
#include <optional>

#include <snail/analysis/stack_provider.hpp>

#include <snail/common/hash_combine.hpp>

using namespace snail;
using namespace snail::analysis;

namespace std {

template<typename T1, typename T2>
struct hash<std::pair<T1, T2>>
{
    [[nodiscard]] size_t operator()(const std::pair<T1, T2>& value) const noexcept
    {
        return common::hash_combine(first_hasher(value.first), second_hasher(value.second));
    }

private:
    std::hash<T1> first_hasher;
    std::hash<T2> second_hasher;
};

} // namespace std

module_info& get_or_create_module(std::vector<module_info>&                           modules,
                                  std::unordered_map<std::string, module_info::id_t>& modules_by_name,
                                  std::string_view                                    module_name)
{
    const auto key = std::string(module_name);

    auto iter = modules_by_name.find(key);
    if(iter == modules_by_name.end())
    {
        modules.push_back(module_info{
            .id   = modules.size(),
            .name = std::string(module_name),
        });
        modules_by_name[key] = modules.back().id;
        return modules.back();
    }

    return modules[iter->second];
}

function_info& get_or_create_function(std::vector<function_info>&                                                       functions,
                                      std::unordered_map<std::pair<module_info::id_t, std::string>, module_info::id_t>& functions_by_name,
                                      const module_info&                                                                module,
                                      std::string_view                                                                  function_name)
{
    const auto key = std::make_pair(module.id, std::string(function_name));

    auto iter = functions_by_name.find(key);
    if(iter == functions_by_name.end())
    {
        functions.push_back(function_info{
            .id        = functions.size(),
            .module_id = module.id,
            .name      = std::string(function_name),
        });
        functions_by_name[key] = functions.back().id;
        return functions.back();
    }

    return functions[iter->second];
}

call_tree_node& get_or_append_call_tree_child(std::vector<call_tree_node>& call_tree_nodes,
                                              call_tree_node&              current_node,
                                              const function_info&         function)
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
    });
    return call_tree_nodes.back();
}

stacks_analysis snail::analysis::analyze_stacks(const analysis::stack_provider& stack_provider,
                                                common::process_id_t            process_id)
{
    std::unordered_map<std::string, module_info::id_t>                               modules_by_name;
    std::unordered_map<std::pair<module_info::id_t, std::string>, module_info::id_t> functions_by_name;

    stacks_analysis result;
    result.process = process_info{
        .process_id = process_id,
        .name       = "???"};

    result.function_root = function_info{
        .id        = std::size_t(-1),
        .module_id = std::size_t(-1),
        .name      = "root"};

    result.call_tree_root = call_tree_node{
        .id          = std::size_t(-1),
        .function_id = result.function_root.id,
    };

    for(const auto& sample : stack_provider.samples(process_id))
    {
        ++result.call_tree_root.hits.total;
        ++result.function_root.hits.total;

        std::optional<module_info::id_t>   previous_node_id;
        std::optional<module_info::id_t>   previous_module_id;
        std::optional<function_info::id_t> previous_function_id;

        for(const auto& stack_entry : sample.reversed_stack())
        {
            auto& module   = get_or_create_module(result.modules, modules_by_name, stack_entry.module_name());
            auto& function = get_or_create_function(result.functions, functions_by_name, module, stack_entry.symbol_name());
            auto& node     = get_or_append_call_tree_child(result.call_tree_nodes, previous_node_id ? result.call_tree_nodes[*previous_node_id] : result.call_tree_root, function);

            ++module.hits.total;
            ++function.hits.total;
            ++node.hits.total;

            if(previous_function_id)
            {
                auto& previous_function = result.functions[*previous_function_id];
                ++previous_function.callees[function.id].total;
                ++function.callers[previous_function.id].total;
            }
            else
            {
                auto& previous_function = result.function_root;
                ++previous_function.callees[function.id].total;
                ++function.callers[previous_function.id].total;
            }

            previous_module_id   = module.id;
            previous_function_id = function.id;
            previous_node_id     = node.id;
        }

        // final elements at the top of the stack have self hits
        if(previous_module_id) ++result.modules[*previous_module_id].hits.self;
        if(previous_function_id) ++result.functions[*previous_function_id].hits.self;
        if(previous_node_id) ++result.call_tree_nodes[*previous_node_id].hits.self;
        else ++result.call_tree_root.hits.self;
    }

    return result;
}