#include <snail/server/requests/requests.hpp>

#include <tuple>

#include <snail/jsonrpc/request.hpp>
#include <snail/jsonrpc/server.hpp>

#include <snail/server/storage/storage.hpp>

#include <snail/data/call_tree.hpp>

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
                        data::process_id_t, process_id);

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

    server.register_request<retrieve_call_tree_hot_path_request>(
        [&](const retrieve_call_tree_hot_path_request& request) -> nlohmann::json
        {
            const auto& call_tree = storage.retrieve_call_tree(document_id{request.document_id()}, request.process_id());

            const auto* current_node = &call_tree.root();

            const auto& root_hits = current_node->hits;

            auto result = nlohmann::json{
                {"name",          current_node->name                                      },
                {"module",        "[multiple]"                                            },
                {"type",          "process"                                               },
                {"total_samples", current_node->hits.total                                },
                {"self_samples",  current_node->hits.self                                 },
                {"total_percent", (double)current_node->hits.total * 100 / root_hits.total},
                {"self_percent",  (double)current_node->hits.self * 100 / root_hits.total },
                {"is_hot",        true                                                    }
            };

            auto* current_json_node = &result;

            while(true)
            {
                const auto* const top_child = current_node->top_total_hit_child();

                auto& children = (*current_json_node)["children"];
                children       = nlohmann::json::array();

                const auto expand_top_child = top_child->hits.total > current_node->hits.self;

                std::size_t top_child_index = 0;
                for(std::size_t i = 0; i < current_node->number_of_children(); ++i)
                {
                    const auto& child = current_node->child(i);

                    const auto is_top_child = &child == top_child;

                    children.push_back({
                        {"name",          child.name                                      },
                        {"module",        "unknown"                                       },
                        {"type",          "function"                                      },
                        {"total_samples", child.hits.total                                },
                        {"self_samples",  child.hits.self                                 },
                        {"total_percent", (double)child.hits.total * 100 / root_hits.total},
                        {"self_percent",  (double)child.hits.self * 100 / root_hits.total },
                        {"is_hot",        is_top_child                                    }
                    });

                    if(is_top_child)
                    {
                        top_child_index = i;
                        if(!expand_top_child)
                        {
                            children.back()["children"] = nlohmann::json(nullptr);
                        }
                    }
                    else
                    {
                        if(child.number_of_children() > 0)
                        {
                            children.back()["children"] = nlohmann::json(nullptr);
                        }
                        else
                        {
                            children.back()["children"] = nlohmann::json::array();
                        }
                    }
                }

                if(top_child == nullptr) break;

                if(!expand_top_child) break;

                current_json_node = &children[top_child_index];
                current_node      = top_child;
            }

            return {
                {"root", std::move(result)}
            };
        });
}
