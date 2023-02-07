#include <snail/server/requests/requests.hpp>

#include <tuple>

#include <snail/jsonrpc/request.hpp>
#include <snail/jsonrpc/server.hpp>

#include <snail/server/storage/storage.hpp>

#include <snail/analysis/analysis.hpp>

using namespace snail;
using namespace snail::server;

SNAIL_JSONRPC_REQUEST_0(initialize);

SNAIL_JSONRPC_REQUEST_1(read_document,
                        std::string_view, file_path);

SNAIL_JSONRPC_REQUEST_1(close_document,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_1(retrieve_processes,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_2(retrieve_call_tree_hot_path,
                        std::size_t, document_id,
                        common::process_id_t, process_id);

SNAIL_JSONRPC_REQUEST_2(retrieve_hottest_function,
                        std::size_t, document_id,
                        common::process_id_t, process_id);

SNAIL_JSONRPC_REQUEST_3(expand_call_tree_node,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        analysis::call_tree_node::id_t, node_id);

SNAIL_JSONRPC_REQUEST_4(retrieve_callers_callees,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        analysis::function_info::id_t, function_id,
                        std::size_t, max_entries);

namespace {

const analysis::call_tree_node* get_top_child(const analysis::stacks_analysis& analysis_result,
                                              const analysis::call_tree_node&  current_node)
{
    const analysis::call_tree_node* top_child = nullptr;
    for(const auto child_id : current_node.children)
    {
        const auto& child = analysis_result.call_tree_nodes[child_id];
        if(top_child == nullptr || child.hits.total > top_child->hits.total)
        {
            top_child = &child;
        }
    }
    return top_child;
}

auto append_call_tree_node_children(const analysis::stacks_analysis& analysis_result,
                                    const analysis::call_tree_node&  current_node,
                                    std::size_t                      total_hits,
                                    nlohmann::json&                  children,
                                    bool                             expand_hot_path)
{
    children = nlohmann::json::array();

    const auto* const top_child = get_top_child(analysis_result, current_node);

    const auto expand_top_child = expand_hot_path &&
                                  top_child != nullptr &&
                                  top_child->hits.total > current_node.hits.self;

    std::size_t top_child_index = 0;
    for(std::size_t child_index = 0; child_index < current_node.children.size(); ++child_index) // TODO: views::enumerate
    {
        const auto  child_id       = current_node.children[child_index];
        const auto& child          = analysis_result.call_tree_nodes[child_id];
        const auto& child_function = analysis_result.functions[child.function_id];
        const auto& child_module   = analysis_result.modules[child_function.module_id];

        assert(top_child != nullptr);
        const auto is_top_child = child.id == top_child->id;

        children.push_back({
            {"name",          child_function.name                        },
            {"id",            child.id                                   },
            {"module",        child_module.name                          },
            {"type",          "function"                                 },
            {"total_samples", child.hits.total                           },
            {"self_samples",  child.hits.self                            },
            {"total_percent", (double)child.hits.total * 100 / total_hits},
            {"self_percent",  (double)child.hits.self * 100 / total_hits },
            {"is_hot",        expand_top_child && is_top_child           }
        });

        if(is_top_child) top_child_index = child_index;

        if(child.children.empty())
        {
            children.back()["children"] = nlohmann::json::array();
        }
        else if(!is_top_child || !expand_top_child)
        {
            children.back()["children"] = nlohmann::json(nullptr);
        }
    }

    return std::make_tuple(top_child, top_child_index, expand_top_child);
}

auto make_function_json(const analysis::stacks_analysis& analysis_result,
                        const analysis::function_info&   function,
                        std::size_t                      total_hits,
                        const analysis::hit_counts*      hits = nullptr)
{
    const auto& module = analysis_result.modules.at(function.module_id);

    if(hits == nullptr) hits = &function.hits;

    return nlohmann::json{
        {"name",          function.name                         },
        {"id",            function.id                           },
        {"module",        module.name                           },
        {"type",          "function"                            },
        {"total_samples", hits->total                           },
        {"self_samples",  hits->self                            },
        {"total_percent", (double)hits->total * 100 / total_hits},
        {"self_percent",  (double)hits->self * 100 / total_hits },
    };
}

} // namespace

void snail::server::register_all(snail::jsonrpc::server& server, snail::server::storage& storage)
{
    server.register_request<initialize_request>(
        [&](const initialize_request&) -> nlohmann::json
        {
            return {
                {"success", true}
            };
        });

    server.register_request<read_document_request>(
        [&](const read_document_request& request) -> nlohmann::json
        {
            const auto document_id = storage.read_document(request.file_path());
            return {
                {"document_id", document_id.id_}
            };
        });

    server.register_notification<close_document_request>(
        [&](const close_document_request& request)
        {
            storage.close_document(document_id{request.document_id()});
        });

    server.register_request<retrieve_processes_request>(
        [&](const retrieve_processes_request& request) -> nlohmann::json
        {
            const auto& processes = storage.retrieve_processes(document_id{request.document_id()});

            auto json_processes = nlohmann::json::array();
            for(const auto& process : processes)
            {
                json_processes.push_back({
                    {"id",   process.process_id},
                    {"name", process.name      }
                });
            }
            return {
                {"processes", std::move(json_processes)}
            };
        });

    server.register_request<retrieve_hottest_function_request>(
        [&](const retrieve_hottest_function_request& request) -> nlohmann::json
        {
            const auto& analysis_result = storage.get_analysis_result(document_id{request.document_id()}, request.process_id());

            const analysis::function_info* hottest_function = nullptr;

            for(const auto& function : analysis_result.functions)
            {
                if(hottest_function == nullptr || function.hits.self > hottest_function->hits.self)
                {
                    hottest_function = &function;
                }
            }

            return {
                {"function_id", hottest_function == nullptr ? nlohmann::json(nullptr) : nlohmann::json(hottest_function->id)}
            };
        });
    server.register_request<retrieve_call_tree_hot_path_request>(
        [&](const retrieve_call_tree_hot_path_request& request) -> nlohmann::json
        {
            const auto& analysis_result = storage.get_analysis_result(document_id{request.document_id()}, request.process_id());

            const auto total_hits = analysis_result.call_tree_root.hits.total; // TODO: this should probably be the total over all processes!

            const auto root_name = std::format("{} (PID: {})", analysis_result.process.name, analysis_result.process.process_id);

            const auto* current_node = &analysis_result.call_tree_root;

            auto result = nlohmann::json{
                {"name",          root_name                                          },
                {"id",            current_node->id                                   },
                {"module",        "[multiple]"                                       },
                {"type",          "process"                                          },
                {"total_samples", current_node->hits.total                           },
                {"self_samples",  current_node->hits.self                            },
                {"total_percent", (double)current_node->hits.total * 100 / total_hits},
                {"self_percent",  (double)current_node->hits.self * 100 / total_hits },
                {"is_hot",        true                                               }
            };

            auto* current_json_node = &result;

            while(true)
            {
                auto& children = (*current_json_node)["children"];

                const auto [top_child, top_child_index, expand_top_child] = append_call_tree_node_children(analysis_result, *current_node, total_hits, children, true);

                if(top_child == nullptr) break;

                if(!expand_top_child) break;

                current_json_node = &children[top_child_index];
                current_node      = top_child;
            }

            return {
                {"root", std::move(result)}
            };
        });
    server.register_request<expand_call_tree_node_request>(
        [&](const expand_call_tree_node_request& request) -> nlohmann::json
        {
            const auto& analysis_result = storage.get_analysis_result(document_id{request.document_id()}, request.process_id());

            const auto total_hits = analysis_result.call_tree_root.hits.total; // TODO: this should probably be the total over all processes!

            const auto& current_node = analysis_result.call_tree_nodes.at(request.node_id());

            nlohmann::json children;
            append_call_tree_node_children(analysis_result, current_node, total_hits, children, false);

            return {
                {"children", std::move(children)}
            };
        });
    server.register_request<retrieve_callers_callees_request>(
        [&](const retrieve_callers_callees_request& request) -> nlohmann::json
        {
            const auto& analysis_result = storage.get_analysis_result(document_id{request.document_id()}, request.process_id());

            const auto max_entries = request.max_entries() > 0 ? request.max_entries() : 1;

            const auto total_hits = analysis_result.call_tree_root.hits.total; // TODO: this should probably be the total over all processes!

            const auto& function = analysis_result.functions.at(request.function_id());

            auto function_json = make_function_json(analysis_result, function, total_hits);

            std::vector<nlohmann::json> callers;
            std::vector<nlohmann::json> callees;

            callers.reserve(function.callers.size());
            for(const auto& [caller_id, caller_hits] : function.callers)
            {
                if(caller_id == analysis_result.function_root.id)
                {
                    const auto root_name = std::format("{} (PID: {})", analysis_result.process.name, analysis_result.process.process_id);

                    callers.push_back(
                        nlohmann::json{
                            {"name",          root_name                                   },
                            {"id",            function.id                                 },
                            {"module",        "[multiple]"                                },
                            {"type",          "process"                                   },
                            {"total_samples", caller_hits.total                           },
                            {"self_samples",  caller_hits.self                            },
                            {"total_percent", (double)caller_hits.total * 100 / total_hits},
                            {"self_percent",  (double)caller_hits.self * 100 / total_hits },
                    });
                }
                else
                {
                    const auto& caller = analysis_result.functions.at(caller_id);
                    callers.push_back(make_function_json(analysis_result, caller, total_hits, &caller_hits));
                }
            }

            std::sort(
                callers.begin(), callers.end(),
                [](const nlohmann::json& lhs, const nlohmann::json& rhs)
                {
                    return lhs.at("total_samples").get<std::size_t>() > rhs.at("total_samples").get<std::size_t>();
                });

            if(callers.size() > max_entries)
            {
                analysis::hit_counts accumulated_hits;
                for(std::size_t i = max_entries - 1; i < callers.size(); ++i)
                {
                    accumulated_hits.total += callers[i].at("total_samples").get<std::size_t>();
                    accumulated_hits.total += callers[i].at("self_samples").get<std::size_t>();
                }

                callers.erase(callers.begin() + (max_entries - 1), callers.end());
                callers.push_back(
                    nlohmann::json{
                        {"name",          "[others]"                                       },
                        {"id",            nlohmann::json(nullptr)                          },
                        {"module",        "[multiple]"                                     },
                        {"type",          "accumulated"                                    },
                        {"total_samples", accumulated_hits.total                           },
                        {"self_samples",  accumulated_hits.self                            },
                        {"total_percent", (double)accumulated_hits.total * 100 / total_hits},
                        {"self_percent",  (double)accumulated_hits.self * 100 / total_hits },
                });
            }

            callees.reserve(function.callees.size());
            for(const auto& [callee_id, callee_hits] : function.callees)
            {
                const auto& callee = analysis_result.functions.at(callee_id);
                callees.push_back(make_function_json(analysis_result, callee, total_hits, &callee_hits));
            }

            std::sort(
                callees.begin(), callees.end(),
                [](const nlohmann::json& lhs, const nlohmann::json& rhs)
                {
                    return lhs.at("total_samples").get<std::size_t>() > rhs.at("total_samples").get<std::size_t>();
                });

            if(callees.size() > max_entries)
            {
                analysis::hit_counts accumulated_hits;
                for(std::size_t i = max_entries - 1; i < callees.size(); ++i)
                {
                    accumulated_hits.total += callees[i].at("total_samples").get<std::size_t>();
                    accumulated_hits.total += callees[i].at("self_samples").get<std::size_t>();
                }

                callees.erase(callees.begin() + (max_entries - 1), callees.end());
                callees.push_back(
                    nlohmann::json{
                        {"name",          "[others]"                                       },
                        {"id",            nlohmann::json(nullptr)                          },
                        {"module",        "[multiple]"                                     },
                        {"type",          "accumulated"                                    },
                        {"total_samples", accumulated_hits.total                           },
                        {"self_samples",  accumulated_hits.self                            },
                        {"total_percent", (double)accumulated_hits.total * 100 / total_hits},
                        {"self_percent",  (double)accumulated_hits.self * 100 / total_hits },
                });
            }

            return {
                {"function", std::move(function_json)          },
                {"callers",  nlohmann::json(std::move(callers))},
                {"callees",  nlohmann::json(std::move(callees))}
            };
        });
}
