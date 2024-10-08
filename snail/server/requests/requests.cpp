#include <snail/server/requests/requests.hpp>

#include <algorithm>
#include <format>
#include <iostream>
#include <sstream>
#include <tuple>

#include <snail/common/cast.hpp>
#include <snail/common/wildcard.hpp>

#include <snail/jsonrpc/request.hpp>
#include <snail/jsonrpc/server.hpp>

#include <snail/server/storage/storage.hpp>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>

#include <snail/server/requests/protocol.hpp>

using namespace snail;
using namespace snail::server;

namespace {

template<std::integral T, std::integral U>
double make_percent(T value, U max_value)
{
    return max_value == 0 ? 0.0 : (static_cast<double>(value) * 100.0 / static_cast<double>(max_value));
}

auto make_hits_json(const analysis::source_hit_counts&                                         hits_per_source,
                    const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& total_hits)
{
    auto results = nlohmann::json::array();

    for(analysis::sample_source_info::id_t source_id = 0; source_id < hits_per_source.counts_per_source.size(); ++source_id)
    {
        const auto& hits   = hits_per_source.counts_per_source[source_id];
        const auto& totals = total_hits.at(source_id);

        results.push_back({
            {"sourceId", source_id},
            {"totalSamples", hits.total},
            {"selfSamples", hits.self},
            {"totalPercent", make_percent(hits.total, totals)},
            {"selfPercent", make_percent(hits.self, totals)},
        });
    }
    return results;
}

const analysis::call_tree_node* get_top_child(const analysis::stacks_analysis&   stacks_analysis,
                                              const analysis::call_tree_node&    current_node,
                                              analysis::sample_source_info::id_t source_id)
{
    const analysis::call_tree_node* top_child = nullptr;
    for(const auto child_id : current_node.children)
    {
        const auto& child = stacks_analysis.get_call_tree_node(child_id);
        if(top_child == nullptr || child.hits.get(source_id).total > top_child->hits.get(source_id).total)
        {
            top_child = &child;
        }
    }
    return top_child;
}

auto append_call_tree_node_children(const analysis::stacks_analysis&                                           stacks_analysis,
                                    const analysis::call_tree_node&                                            current_node,
                                    const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& total_hits,
                                    nlohmann::json&                                                            children,
                                    bool                                                                       expand_hot_path = false,
                                    std::optional<analysis::sample_source_info::id_t>                          hot_source_id   = std::nullopt)
{
    children = nlohmann::json::array();

    const auto* const top_child = hot_source_id ? get_top_child(stacks_analysis, current_node, *hot_source_id) : nullptr;

    const auto expand_top_child = expand_hot_path &&
                                  top_child != nullptr &&
                                  hot_source_id &&
                                  top_child->hits.get(*hot_source_id).total > current_node.hits.get(*hot_source_id).self;

    std::size_t top_child_index = 0;
    for(std::size_t child_index = 0; child_index < current_node.children.size(); ++child_index) // TODO: views::enumerate
    {
        const auto  child_id       = current_node.children[child_index];
        const auto& child          = stacks_analysis.get_call_tree_node(child_id);
        const auto& child_function = stacks_analysis.get_function(child.function_id);
        const auto& child_module   = stacks_analysis.get_module(child_function.module_id);

        const auto is_top_child = top_child != nullptr && child.id == top_child->id;

        children.push_back({
            {"name", child_function.name},
            {"id", child.id},
            {"functionId", child.function_id},
            {"module", child_module.name},
            {"type", "function"},
            {"hits", make_hits_json(child.hits, total_hits)},
            {"isHot", expand_top_child && is_top_child}
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

auto make_function_json(const analysis::stacks_analysis&                                           stacks_analysis,
                        const analysis::process_info&                                              process,
                        const analysis::function_info&                                             function,
                        const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& total_hits,
                        const analysis::source_hit_counts*                                         hits = nullptr)
{
    if(hits == nullptr) hits = &function.hits;

    const auto is_root = function.id == stacks_analysis.get_function_root().id;

    auto function_name = is_root ? std::format("{} (PID: {})", process.name, process.os_id) :
                                   function.name;

    auto module_name = is_root ? "[multiple]" :
                                 stacks_analysis.get_module(function.module_id).name;

    const auto* const type = is_root ? "process" : "function";

    return nlohmann::json{
        {"name", std::move(function_name)},
        {"id", function.id},
        {"module", std::move(module_name)},
        {"type", type},
        {"hits", make_hits_json(*hits, total_hits)}
    };
}

auto make_callers_callees_json(const analysis::stacks_analysis&                                                      stacks_analysis,
                               const analysis::process_info&                                                         process,
                               const auto                                                                            max_entries,
                               const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>&            total_hits,
                               analysis::sample_source_info::id_t                                                    sort_source_id,
                               const std::unordered_map<analysis::function_info::id_t, analysis::source_hit_counts>& functions)
{
    assert(max_entries >= 1);
    struct id_and_count
    {
        analysis::function_info::id_t id;
        std::size_t                   count;
    };

    std::vector<id_and_count> counts_per_function;
    for(const auto& [id, hits] : functions)
    {
        counts_per_function.push_back(
            {.id    = id,
             .count = hits.get(sort_source_id).total});
    }

    std::sort(
        counts_per_function.begin(), counts_per_function.end(),
        [](const id_and_count& lhs, const id_and_count& rhs)
        {
            return lhs.count > rhs.count;
        });

    auto result = nlohmann::json::array();

    const auto need_accumulated_item = counts_per_function.size() > max_entries;

    const auto regular_items_count = need_accumulated_item ? max_entries - 1 : counts_per_function.size();

    // result.reserve(functions.size());
    for(const auto& [id, count] : std::views::take(counts_per_function, regular_items_count))
    {
        const auto& function = stacks_analysis.get_function(id);
        result.push_back(make_function_json(stacks_analysis, process, function, total_hits));
    }

    if(need_accumulated_item)
    {
        analysis::source_hit_counts accumulated_hits;
        for(const auto& [id, count] : std::views::drop(counts_per_function, regular_items_count))
        {
            const auto& function = stacks_analysis.get_function(id);

            for(analysis::sample_source_info::id_t source_id = 0; source_id < function.hits.counts_per_source.size(); ++source_id)
            {
                const auto& hits = function.hits.counts_per_source[source_id];

                accumulated_hits.get(source_id).total = hits.total;
                accumulated_hits.get(source_id).self  = hits.self;
            }
        }

        result.push_back(
            nlohmann::json{
                {"name", "[others]"},
                {"id", nlohmann::json(nullptr)},
                {"module", "[multiple]"},
                {"type", "accumulated"},
                {"hits", make_hits_json(accumulated_hits, total_hits)},
        });
    }
    return result;
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
    server.register_request<shutdown_request>(
        [&](const shutdown_request&) -> nlohmann::json
        {
            std::cout << "Shutting down...\n";
            return nullptr;
        });
    server.register_notification<exit_request>(
        [&](const exit_request&)
        {
            std::exit(EXIT_SUCCESS);
        });

    server.register_notification<set_pdb_symbol_find_options_request>(
        [&](const set_pdb_symbol_find_options_request& request)
        {
            auto& find_options = storage.get_options().pdb_find_options;

            find_options = analysis::pdb_symbol_find_options();

            if(request.symbol_cache_dir()) find_options.symbol_cache_dir_ = *request.symbol_cache_dir();

            for(const auto& seach_dir : request.search_dirs())
            {
                find_options.search_dirs_.push_back(seach_dir);
            }

            if(request.no_default_urls()) find_options.symbol_server_urls_.clear();
            for(const auto& url : request.symbol_server_urls())
            {
                find_options.symbol_server_urls_.push_back(url);
            }
        });

    server.register_notification<set_dwarf_symbol_find_options_request>(
        [&](const set_dwarf_symbol_find_options_request& request)
        {
            auto& find_options = storage.get_options().dwarf_find_options;

            find_options = analysis::dwarf_symbol_find_options();

            if(request.debuginfod_cache_dir()) find_options.debuginfod_cache_dir_ = *request.debuginfod_cache_dir();

            for(const auto& seach_dir : request.search_dirs())
            {
                find_options.search_dirs_.push_back(seach_dir);
            }

            if(request.no_default_urls()) find_options.debuginfod_urls_.clear();
            for(const auto& url : request.debuginfod_urls())
            {
                find_options.debuginfod_urls_.push_back(url);
            }
        });

    server.register_notification<set_module_path_maps_request>(
        [&](const set_module_path_maps_request& request)
        {
            auto& module_path_map = storage.get_module_path_map();

            module_path_map = analysis::path_map();
            for(const auto& [source, target] : request.simple_maps())
            {
                module_path_map.add_rule(std::make_unique<analysis::simple_path_mapper>(
                    source, target));
            }
        });

    server.register_notification<set_module_filters_request>(
        [&](const set_module_filters_request& request)
        {
            auto& filter = storage.get_options().filter;

            switch(request.mode())
            {
            case module_filter_mode::all_but_excluded:
                filter.mode = analysis::filter_mode::all_but_excluded;
                break;
            case module_filter_mode::only_included:
                filter.mode = analysis::filter_mode::only_included;
                break;
            }

            for(const auto& wildcard_str : request.include())
            {
                filter.included.emplace_back(common::wildcard_to_regex(wildcard_str));
            }
            for(const auto& wildcard_str : request.exclude())
            {
                filter.excluded.emplace_back(common::wildcard_to_regex(wildcard_str));
            }
        });

    server.register_request<read_document_request>(
        [&](const read_document_request& request) -> nlohmann::json
        {
            const auto document_id = storage.read_document(request.file_path());
            return {
                {"documentId", document_id.id_}
            };
        });

    server.register_notification<close_document_request>(
        [&](const close_document_request& request)
        {
            storage.close_document({request.document_id()});
        });

    server.register_request<set_sample_filters_request>(
        [&](const set_sample_filters_request& request) -> nlohmann::json
        {
            analysis::sample_filter filter;

            if(request.min_time()) filter.min_time = std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(*request.min_time()));
            if(request.max_time()) filter.max_time = std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(*request.max_time()));

            for(const auto thread_key : request.excluded_threads())
            {
                filter.excluded_threads.insert(analysis::unique_thread_id{thread_key});
            }
            for(const auto process_key : request.excluded_processes())
            {
                filter.excluded_processes.insert(analysis::unique_process_id{process_key});
            }

            storage.apply_document_filter({request.document_id()}, std::move(filter));

            return nullptr;
        });

    server.register_request<retrieve_sample_sources_request>(
        [&](const retrieve_sample_sources_request& request) -> nlohmann::json
        {
            const auto& data_provider = storage.get_data({request.document_id()});

            auto json_sample_sources = nlohmann::json::array();
            for(const auto& source_info : data_provider.sample_sources())
            {
                json_sample_sources.push_back({
                    {"id",                  source_info.id                   },
                    {"name",                source_info.name                 },
                    {"numberOfSamples",     source_info.number_of_samples    },
                    {"averageSamplingRate", source_info.average_sampling_rate},
                    {"hasStacks",           source_info.has_stacks           },
                });
            }

            return {
                {"sampleSources", std::move(json_sample_sources)}
            };
        });

    server.register_request<retrieve_session_info_request>(
        [&](const retrieve_session_info_request& request) -> nlohmann::json
        {
            const auto& data_provider = storage.get_data({request.document_id()});

            const auto& session_info = data_provider.session_info();

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
                {"sessionInfo",
                 {
                     {"commandLine", session_info.command_line},
                     {"date", format_date(session_info.date)},
                     {"runtime", session_info.runtime.count()},
                     {"numberOfProcesses", session_info.number_of_processes},
                     {"numberOfThreads", session_info.number_of_threads},
                     {"numberOfSamples", session_info.number_of_samples},
                 }}
            };
        });
    server.register_request<retrieve_system_info_request>(
        [&](const retrieve_system_info_request& request) -> nlohmann::json
        {
            const auto& system_info = storage.get_data({request.document_id()}).system_info();

            return {
                {"systemInfo",
                 {{"hostname", system_info.hostname},
                  {"platform", system_info.platform},
                  {"architecture", system_info.architecture},
                  {"cpuName", system_info.cpu_name},
                  {"numberOfProcessors", system_info.number_of_processors}}}
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
                        {"key",       thread_info.unique_id.key                                                     },
                        {"osId",      thread_info.os_id                                                             },
                        {"name",      thread_info.name ? nlohmann::json(*thread_info.name) : nlohmann::json(nullptr)},
                        {"startTime", thread_info.start_time.count()                                                },
                        {"endTime",   thread_info.end_time.count()                                                  }
                    });
                }

                const auto& process_info = data_provider.process_info(process_id);
                json_processes.push_back({
                    {"key",       process_info.unique_id.key     },
                    {"osId",      process_info.os_id             },
                    {"name",      process_info.name              },
                    {"startTime", process_info.start_time.count()},
                    {"endTime",   process_info.end_time.count()  },
                    {"threads",   std::move(json_threads)        }
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
                analysis::unique_process_id    process_id;
                const analysis::function_info* function;
            };

            std::vector<intermediate_function_info> intermediate_functions;

            const auto& data_provider = storage.get_data({request.document_id()});

            const auto source_id = request.source_id();

            const auto sort_by = sort_by_samples{
                .sample_source_id = source_id,
                .sum              = sort_by_samples::sum_type::self,
            };

            for(const auto process_id : data_provider.sampling_processes())
            {
                const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, process_id);
                const auto& function_ids    = storage.get_functions_page({request.document_id()}, process_id,
                                                                         sort_by,
                                                                         true,
                                                                         request.count(), 0);

                for(const auto function_id : function_ids)
                {
                    intermediate_functions.push_back(intermediate_function_info{
                        .process_id = process_id,
                        .function   = &stacks_analysis.get_function(function_id)});
                }
            }

            std::ranges::sort(intermediate_functions,
                              [source_id](const intermediate_function_info& lhs, const intermediate_function_info& rhs)
                              {
                                  return lhs.function->hits.get(source_id).self > rhs.function->hits.get(source_id).self;
                              });

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            auto json_functions = nlohmann::json::array();
            for(const auto& entry : intermediate_functions | std::views::take(request.count()))
            {
                const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, entry.process_id);
                const auto& process_info    = data_provider.process_info(entry.process_id);
                json_functions.push_back({
                    {"processKey", entry.process_id.key},
                    {"function", make_function_json(stacks_analysis, process_info, *entry.function, total_hits)}
                });
            }

            return {
                {"functions", std::move(json_functions)}
            };
        });

    server.register_request<retrieve_functions_page_request>(
        [&](const retrieve_functions_page_request& request) -> nlohmann::json
        {
            const auto sort_by = [&request]() -> sort_by_kind
            {
                switch(request.sort_by())
                {
                default:
                case functions_sort_by::name:
                    return sort_by_name{};
                case functions_sort_by::self_samples:
                    if(!request.sort_source_id()) throw jsonrpc::invalid_request_error("missing 'sortSourceId'.");
                    return sort_by_samples{
                        .sample_source_id = *request.sort_source_id(),
                        .sum              = sort_by_samples::sum_type::self};
                case functions_sort_by::total_samples:
                    if(!request.sort_source_id()) throw jsonrpc::invalid_request_error("missing 'sortSourceId'.");
                    return sort_by_samples{
                        .sample_source_id = *request.sort_source_id(),
                        .sum              = sort_by_samples::sum_type::total};
                }
            }();

            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, {request.process_key()});
            const auto& function_ids    = storage.get_functions_page({request.document_id()}, {request.process_key()},
                                                                     sort_by, request.sort_order() == sort_direction::descending,
                                                                     request.page_size(), request.page_index());

            const auto& data_provider = storage.get_data({request.document_id()});

            const auto& process_info = data_provider.process_info({request.process_key()});

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            auto json_functions = nlohmann::json::array();
            if(request.sort_order() == sort_direction::descending)
            {
                for(const auto function_id : std::views::reverse(function_ids))
                {
                    json_functions.push_back(make_function_json(stacks_analysis, process_info, stacks_analysis.get_function(function_id), total_hits));
                }
            }
            else
            {
                for(const auto function_id : function_ids)
                {
                    json_functions.push_back(make_function_json(stacks_analysis, process_info, stacks_analysis.get_function(function_id), total_hits));
                }
            }

            return {
                {"functions", json_functions}
            };
        });
    server.register_request<retrieve_call_tree_hot_path_request>(
        [&](const retrieve_call_tree_hot_path_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, {request.process_key()});

            const auto& data_provider = storage.get_data({request.document_id()});
            const auto& process       = data_provider.process_info({request.process_key()});

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            const auto root_name = std::format("{} (PID: {})", process.name, process.os_id);

            const auto* current_node = &stacks_analysis.get_call_tree_root();

            auto result = nlohmann::json{
                {"name", root_name},
                {"id", current_node->id},
                {"functionId", current_node->function_id},
                {"module", "[multiple]"},
                {"type", "process"},
                {"hits", make_hits_json(current_node->hits, total_hits)},
                {"isHot", true}
            };

            auto* current_json_node = &result;

            while(true)
            {
                auto& children = (*current_json_node)["children"];

                const auto [top_child, top_child_index, expand_top_child] = append_call_tree_node_children(stacks_analysis, *current_node, total_hits, children, true, request.source_id());

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
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, {request.process_key()});

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            const auto& current_node = stacks_analysis.get_call_tree_node(request.node_id());

            nlohmann::json children;
            append_call_tree_node_children(stacks_analysis, current_node, total_hits, children);

            return {
                {"children", std::move(children)}
            };
        });
    server.register_request<retrieve_callers_callees_request>(
        [&](const retrieve_callers_callees_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, {request.process_key()});

            const auto& data_provider = storage.get_data({request.document_id()});
            const auto& process       = data_provider.process_info({request.process_key()});

            const auto max_entries = request.max_entries() > 0 ? request.max_entries() : 1;

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            const auto& function = stacks_analysis.get_function(request.function_id());

            auto function_json = make_function_json(stacks_analysis, process, function, total_hits);

            auto callers_json = make_callers_callees_json(stacks_analysis, process, max_entries, total_hits, request.sort_source_id(), function.callers);
            auto callees_json = make_callers_callees_json(stacks_analysis, process, max_entries, total_hits, request.sort_source_id(), function.callees);

            return {
                {"function", std::move(function_json)},
                {"callers",  std::move(callers_json) },
                {"callees",  std::move(callees_json) }
            };
        });
    server.register_request<retrieve_line_info_request>(
        [&](const retrieve_line_info_request& request) -> nlohmann::json
        {
            const auto& stacks_analysis = storage.get_stacks_analysis({request.document_id()}, {request.process_key()});
            const auto& function        = stacks_analysis.get_function(request.function_id());

            if(function.file_id == std::nullopt || function.line_number == std::nullopt)
            {
                return nullptr;
            }

            const auto& total_hits = storage.get_total_samples_counts({request.document_id()});

            const auto& file = stacks_analysis.get_file(*function.file_id);

            std::vector<nlohmann::json> line_hits;

            line_hits.reserve(function.hits_by_line.size());
            for(const auto& [line_number, hits] : function.hits_by_line)
            {
                line_hits.push_back(
                    nlohmann::json{
                        {"lineNumber", line_number},
                        {"hits", make_hits_json(hits, total_hits)},
                });
            }

            return {
                {"filePath", file.path},
                {"hits", make_hits_json(function.hits, total_hits)},
                {"lineNumber", *function.line_number},
                {"lineHits", nlohmann::json(std::move(line_hits))}
            };
        });
}
