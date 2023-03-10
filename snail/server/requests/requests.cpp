#include <snail/server/requests/requests.hpp>

#include <algorithm>
#include <format>
#include <sstream>
#include <tuple>

#include <snail/jsonrpc/request.hpp>
#include <snail/jsonrpc/server.hpp>

#include <snail/server/storage/storage.hpp>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>

using namespace snail;
using namespace snail::server;

SNAIL_JSONRPC_REQUEST_0(initialize);

SNAIL_JSONRPC_REQUEST_1(read_document,
                        std::string_view, file_path);

SNAIL_JSONRPC_REQUEST_1(close_document,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_1(retrieve_session_info,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_1(retrieve_system_info,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_1(retrieve_processes,
                        std::size_t, document_id);

SNAIL_JSONRPC_REQUEST_2(retrieve_hottest_functions,
                        std::size_t, document_id,
                        std::size_t, count);

SNAIL_JSONRPC_REQUEST_2(retrieve_call_tree_hot_path,
                        std::size_t, document_id,
                        common::process_id_t, process_id);

SNAIL_JSONRPC_REQUEST_4(retrieve_functions_page,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        std::size_t, page_size,
                        std::size_t, page_index);

SNAIL_JSONRPC_REQUEST_3(expand_call_tree_node,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        analysis::call_tree_node::id_t, node_id);

SNAIL_JSONRPC_REQUEST_4(retrieve_callers_callees,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        analysis::function_info::id_t, function_id,
                        std::size_t, max_entries);

SNAIL_JSONRPC_REQUEST_3(retrieve_line_info,
                        std::size_t, document_id,
                        common::process_id_t, process_id,
                        analysis::function_info::id_t, function_id);

namespace {

const analysis::call_tree_node* get_top_child(const analysis::stacks_analysis& stacks_analysis,
                                              const analysis::call_tree_node&  current_node)
{
    const analysis::call_tree_node* top_child = nullptr;
    for(const auto child_id : current_node.children)
    {
        const auto& child = stacks_analysis.get_call_tree_node(child_id);
        if(top_child == nullptr || child.hits.total > top_child->hits.total)
        {
            top_child = &child;
        }
    }
    return top_child;
}

auto append_call_tree_node_children(const analysis::stacks_analysis& stacks_analysis,
                                    const analysis::call_tree_node&  current_node,
                                    std::size_t                      total_hits,
                                    nlohmann::json&                  children,
                                    bool                             expand_hot_path)
{
    children = nlohmann::json::array();

    const auto* const top_child = get_top_child(stacks_analysis, current_node);

    const auto expand_top_child = expand_hot_path &&
                                  top_child != nullptr &&
                                  top_child->hits.total > current_node.hits.self;

    std::size_t top_child_index = 0;
    for(std::size_t child_index = 0; child_index < current_node.children.size(); ++child_index) // TODO: views::enumerate
    {
        const auto  child_id       = current_node.children[child_index];
        const auto& child          = stacks_analysis.get_call_tree_node(child_id);
        const auto& child_function = stacks_analysis.get_function(child.function_id);
        const auto& child_module   = stacks_analysis.get_module(child_function.module_id);

        assert(top_child != nullptr);
        const auto is_top_child = child.id == top_child->id;

        children.push_back({
            {"name",          child_function.name                        },
            {"id",            child.id                                   },
            {"function_id",   child.function_id                          },
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

auto make_function_json(const analysis::stacks_analysis& stacks_analysis,
                        const analysis::function_info&   function,
                        std::size_t                      total_hits,
                        const analysis::hit_counts*      hits = nullptr)
{
    if(hits == nullptr) hits = &function.hits;

    const auto is_root = function.id == stacks_analysis.get_function_root().id;

    auto function_name = is_root ? std::format("{} (PID: {})", stacks_analysis.process.name, stacks_analysis.process.id) :
                                   function.name;

    auto module_name = is_root ? "[multiple]" :
                                 stacks_analysis.get_module(function.module_id).name;

    const auto* const type = is_root ? "process" : "function";

    return nlohmann::json{
        {"name",          std::move(function_name)              },
        {"id",            function.id                           },
        {"module",        std::move(module_name)                },
        {"type",          type                                  },
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
            storage.close_document({request.document_id()});
        });

    server.register_request<retrieve_session_info_request>(
        [&](const retrieve_session_info_request& request) -> nlohmann::json
        {
            const auto& session_info = storage.get_data({request.document_id()}).session_info();

            const auto format_date = [](const std::chrono::sys_seconds& date) -> std::string
            {
#if _MSC_VER // Missing compiler support for formatting chrono dates in (at least) clang/libc++
                return std::format("{0:%F}T{0:%R%z}", date);
#else
                std::stringstream buffer;
                const auto        time_since_epoch = std::chrono::system_clock::to_time_t(date);
                const auto        calender_time    = *std::gmtime(&time_since_epoch);
                buffer << std::put_time(&calender_time, "%FT%R%z");
                return buffer.str();
#endif
            };

            return {
                {"session_info",
                 {{"command_line", session_info.command_line},
                  {"date", format_date(session_info.date)},
                  {"runtime", session_info.runtime.count()},
                  {"number_of_processes", session_info.number_of_processes},
                  {"number_of_threads", session_info.number_of_threads},
                  {"number_of_samples", session_info.number_of_samples},
                  {"average_sampling_rate", session_info.average_sampling_rate}}}
            };
        });
    server.register_request<retrieve_system_info_request>(
        [&](const retrieve_system_info_request& request) -> nlohmann::json
        {
            const auto& system_info = storage.get_data({request.document_id()}).system_info();

            return {
                {"system_info",
                 {{"hostname", system_info.hostname},
                  {"platform", system_info.platform},
                  {"architecture", system_info.architecture},
                  {"cpu_name", system_info.cpu_name},
                  {"number_of_processors", system_info.number_of_processors}}}
            };
        });
    server.register_request<retrieve_processes_request>(
        [&](const retrieve_processes_request& request) -> nlohmann::json
        {
            const auto& data_provider = storage.get_data({request.document_id()});

            auto json_processes = nlohmann::json::array();
            for(const auto process_id : data_provider.sampling_processes())
            {
                auto json_threads = nlohmann::json::array();
                for(const auto& thread_info : data_provider.threads_info(process_id))
                {
                    json_threads.push_back({
                        {"id",         thread_info.id                                                                },
                        {"name",       thread_info.name ? nlohmann::json(*thread_info.name) : nlohmann::json(nullptr)},
                        {"start_time", thread_info.start_time                                                        },
                        {"end_time",   thread_info.end_time                                                          }
                    });
                }

                const auto& process_info = data_provider.process_info(process_id);
                json_processes.push_back({
                    {"id",         process_id             },
                    {"name",       process_info.name      },
                    {"start_time", process_info.start_time},
                    {"end_time",   process_info.end_time  },
                    {"threads",    std::move(json_threads)}
                });
            }
            return {
                {"processes", std::move(json_processes)}
            };
        });
    server.register_request<retrieve_hottest_functions_request>(
        [&](const retrieve_hottest_functions_request& request) -> nlohmann::json
        {
            struct intermediate_function_info
            {
                common::process_id_t           process_id;
                const analysis::function_info* function;
            };

            std::vector<intermediate_function_info> intermediate_functions;

            const auto& data_provider = storage.get_data({request.document_id()});

            for(const auto process_id : data_provider.sampling_processes())
            {
                const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, process_id);
                const auto& function_ids    = storage.get_functions_page({request.document_id()}, process_id,
                                                                         function_data_type::self_samples, true,
                                                                         request.count(), 0);

                for(const auto function_id : function_ids)
                {
                    intermediate_functions.push_back(intermediate_function_info{
                        .process_id = process_id,
                        .function   = &stacks_analysis.get_function(function_id)});
                }
            }

            std::ranges::sort(intermediate_functions,
                              [](const intermediate_function_info& lhs, const intermediate_function_info& rhs)
                              {
                                  return lhs.function->hits.self > rhs.function->hits.self;
                              });

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            auto json_functions = nlohmann::json::array();
            for(const auto& entry : intermediate_functions | std::views::take(request.count()))
            {
                const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, entry.process_id);
                json_functions.push_back({
                    {"process_id", entry.process_id},
                    {"function", make_function_json(stacks_analysis, *entry.function, total_hits)}
                });
            }

            return {
                {"functions", std::move(json_functions)}
            };
        });

    server.register_request<retrieve_functions_page_request>(
        [&](const retrieve_functions_page_request& request) -> nlohmann::json
        {
            // TODO: Add `sort_by` and `sort_order` to request
            const auto sort_by    = function_data_type::self_samples;
            const auto sort_order = direction::descending;

            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, request.process_id());
            const auto& function_ids    = storage.get_functions_page({request.document_id()}, request.process_id(),
                                                                     sort_by, sort_order == direction::descending,
                                                                     request.page_size(), request.page_index());

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            auto json_functions = nlohmann::json::array();
            // if(sort_order == direction::descending)
            assert(sort_order == direction::descending);
            {
                for(const auto function_id : std::views::reverse(function_ids))
                {
                    json_functions.push_back(make_function_json(stacks_analysis, stacks_analysis.get_function(function_id), total_hits));
                }
            }
            // else
            // {
            //     for(const auto function_id : function_ids)
            //     {
            //         json_functions.push_back(make_function_json(stacks_analysis, stacks_analysis.get_function(function_id), total_hits));
            //     }
            // }

            return {
                {"functions", json_functions}
            };
        });
    server.register_request<retrieve_call_tree_hot_path_request>(
        [&](const retrieve_call_tree_hot_path_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, request.process_id());

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            const auto root_name = std::format("{} (PID: {})", stacks_analysis.process.name, stacks_analysis.process.id);

            const auto* current_node = &stacks_analysis.get_call_tree_root();

            auto result = nlohmann::json{
                {"name",          root_name                                          },
                {"id",            current_node->id                                   },
                {"function_id",   current_node->function_id                          },
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

                const auto [top_child, top_child_index, expand_top_child] = append_call_tree_node_children(stacks_analysis, *current_node, total_hits, children, true);

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
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, request.process_id());

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            const auto& current_node = stacks_analysis.get_call_tree_node(request.node_id());

            nlohmann::json children;
            append_call_tree_node_children(stacks_analysis, current_node, total_hits, children, false);

            return {
                {"children", std::move(children)}
            };
        });
    server.register_request<retrieve_callers_callees_request>(
        [&](const retrieve_callers_callees_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, request.process_id());

            const auto max_entries = request.max_entries() > 0 ? request.max_entries() : 1;

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            const auto& function = stacks_analysis.get_function(request.function_id());

            auto function_json = make_function_json(stacks_analysis, function, total_hits);

            std::vector<nlohmann::json> callers;
            std::vector<nlohmann::json> callees;

            callers.reserve(function.callers.size());
            for(const auto& [caller_id, caller_hits] : function.callers)
            {
                const auto& caller = stacks_analysis.get_function(caller_id);
                callers.push_back(make_function_json(stacks_analysis, caller, total_hits, &caller_hits));
            }

            std::sort(
                callers.begin(), callers.end(),
                [](const nlohmann::json& lhs, const nlohmann::json& rhs)
                {
                    return lhs.at("total_samples").get<std::size_t>() > rhs.at("total_samples").get<std::size_t>();
                });

            // TODO: make into function and remove code duplication with code below
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
                const auto& callee = stacks_analysis.get_function(callee_id);
                callees.push_back(make_function_json(stacks_analysis, callee, total_hits, &callee_hits));
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
    server.register_request<retrieve_line_info_request>(
        [&](const retrieve_line_info_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, request.process_id());
            const auto& function        = stacks_analysis.get_function(request.function_id());

            if(function.file_id == std::nullopt || function.line_number == std::nullopt)
            {
                return {
                    {"file_path",     nlohmann::json(nullptr)},
                    {"self_samples",  nlohmann::json(nullptr)},
                    {"total_samples", nlohmann::json(nullptr)},
                    {"self_percent",  nlohmann::json(nullptr)},
                    {"total_percent", nlohmann::json(nullptr)},
                    {"line_number",   nlohmann::json(nullptr)},
                    {"line_hits",     nlohmann::json::array()}
                };
            }

            const auto total_hits = storage.get_data({request.document_id()}).session_info().number_of_samples; // TODO: use filtered

            const auto& file = stacks_analysis.get_file(*function.file_id);

            std::vector<nlohmann::json> line_hits;

            for(const auto& [line_number, hits] : function.hits_by_line)
            {
                line_hits.push_back(
                    nlohmann::json{
                        {"line_number",   line_number                          },
                        {"total_samples", hits.total                           },
                        {"self_samples",  hits.self                            },
                        {"total_percent", (double)hits.total * 100 / total_hits},
                        {"self_percent",  (double)hits.self * 100 / total_hits },
                });
            }

            return {
                {"file_path",     file.path                                     },
                {"total_samples", function.hits.total                           },
                {"self_samples",  function.hits.self                            },
                {"total_percent", (double)function.hits.total * 100 / total_hits},
                {"self_percent",  (double)function.hits.self * 100 / total_hits },
                {"line_number",   *function.line_number                         },
                {"line_hits",     nlohmann::json(std::move(line_hits))          }
            };
        });
}
