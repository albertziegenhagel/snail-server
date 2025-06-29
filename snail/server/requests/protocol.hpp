// ****** THIS IS A GENERATED FILE, DO NOT EDIT. ******

#include <snail/jsonrpc/detail/request.hpp>

#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

enum class module_filter_mode
{
    all_but_excluded,
    only_included,
};
namespace snail::jsonrpc::detail {
template<>
struct enum_value_type<module_filter_mode>
{
    using type = std::string_view;
};
template<>
module_filter_mode enum_from_value<module_filter_mode>(const std::string_view& value)
{
    if(value == "all_but_excluded") return module_filter_mode::all_but_excluded;
    if(value == "only_included") return module_filter_mode::only_included;
    throw std::runtime_error(std::format("'{}' is not a valid value for enum 'module_filter_mode'", value));
}
} // namespace snail::jsonrpc::detail

enum class functions_sort_by
{
    name,
    self_samples,
    total_samples,
};
namespace snail::jsonrpc::detail {
template<>
struct enum_value_type<functions_sort_by>
{
    using type = std::string_view;
};
template<>
functions_sort_by enum_from_value<functions_sort_by>(const std::string_view& value)
{
    if(value == "name") return functions_sort_by::name;
    if(value == "self_samples") return functions_sort_by::self_samples;
    if(value == "total_samples") return functions_sort_by::total_samples;
    throw std::runtime_error(std::format("'{}' is not a valid value for enum 'functions_sort_by'", value));
}
} // namespace snail::jsonrpc::detail

enum class sort_direction
{
    ascending,
    descending,
};
namespace snail::jsonrpc::detail {
template<>
struct enum_value_type<sort_direction>
{
    using type = std::string_view;
};
template<>
sort_direction enum_from_value<sort_direction>(const std::string_view& value)
{
    if(value == "ascending") return sort_direction::ascending;
    if(value == "descending") return sort_direction::descending;
    throw std::runtime_error(std::format("'{}' is not a valid value for enum 'sort_direction'", value));
}
} // namespace snail::jsonrpc::detail

using progress_token = std::variant<long long, std::string>;

struct initialize_request
{
    static constexpr std::string_view name = "initialize";

    static constexpr auto parameters = std::tuple();

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<initialize_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct shutdown_request
{
    static constexpr std::string_view name = "shutdown";

    static constexpr auto parameters = std::tuple();

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<shutdown_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct read_document_request
{
    static constexpr std::string_view name = "readDocument";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::string>{"filePath"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::string& file_path() const
    {
        return std::get<0>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<1>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::string,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<read_document_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_sample_sources_request
{
    static constexpr std::string_view name = "retrieveSampleSources";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_sample_sources_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_session_info_request
{
    static constexpr std::string_view name = "retrieveSessionInfo";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_session_info_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_system_info_request
{
    static constexpr std::string_view name = "retrieveSystemInfo";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_system_info_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_processes_request
{
    static constexpr std::string_view name = "retrieveProcesses";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_processes_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_hottest_functions_request
{
    static constexpr std::string_view name = "retrieveHottestFunctions";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"count"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"sourceId"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::size_t& count() const
    {
        return std::get<0>(data_);
    }

    const std::size_t& source_id() const
    {
        return std::get<1>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<2>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t,
        std::size_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_hottest_functions_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_call_tree_hot_path_request
{
    static constexpr std::string_view name = "retrieveCallTreeHotPath";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"sourceId"},
        snail::jsonrpc::detail::request_parameter<std::uint64_t>{"processKey"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::size_t& source_id() const
    {
        return std::get<0>(data_);
    }

    const std::uint64_t& process_key() const
    {
        return std::get<1>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<2>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t,
        std::uint64_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_call_tree_hot_path_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_functions_page_request
{
    static constexpr std::string_view name = "retrieveFunctionsPage";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<functions_sort_by>{"sortBy"},
        snail::jsonrpc::detail::request_parameter<sort_direction>{"sortOrder"},
        snail::jsonrpc::detail::request_parameter<std::optional<std::size_t>>{"sortSourceId"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"pageSize"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"pageIndex"},
        snail::jsonrpc::detail::request_parameter<std::uint64_t>{"processKey"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const functions_sort_by& sort_by() const
    {
        return std::get<0>(data_);
    }

    const sort_direction& sort_order() const
    {
        return std::get<1>(data_);
    }

    const std::optional<std::size_t>& sort_source_id() const
    {
        return std::get<2>(data_);
    }

    const std::size_t& page_size() const
    {
        return std::get<3>(data_);
    }

    const std::size_t& page_index() const
    {
        return std::get<4>(data_);
    }

    const std::uint64_t& process_key() const
    {
        return std::get<5>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<6>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<7>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        functions_sort_by,
        sort_direction,
        std::optional<std::size_t>,
        std::size_t,
        std::size_t,
        std::uint64_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_functions_page_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct expand_call_tree_node_request
{
    static constexpr std::string_view name = "expandCallTreeNode";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"nodeId"},
        snail::jsonrpc::detail::request_parameter<std::uint64_t>{"processKey"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::size_t& node_id() const
    {
        return std::get<0>(data_);
    }

    const std::uint64_t& process_key() const
    {
        return std::get<1>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<2>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t,
        std::uint64_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<expand_call_tree_node_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_callers_callees_request
{
    static constexpr std::string_view name = "retrieveCallersCallees";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"sortSourceId"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"maxEntries"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"functionId"},
        snail::jsonrpc::detail::request_parameter<std::uint64_t>{"processKey"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::size_t& sort_source_id() const
    {
        return std::get<0>(data_);
    }

    const std::size_t& max_entries() const
    {
        return std::get<1>(data_);
    }

    const std::size_t& function_id() const
    {
        return std::get<2>(data_);
    }

    const std::uint64_t& process_key() const
    {
        return std::get<3>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<4>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<5>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t,
        std::size_t,
        std::size_t,
        std::uint64_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_callers_callees_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct retrieve_line_info_request
{
    static constexpr std::string_view name = "retrieveLineInfo";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"functionId"},
        snail::jsonrpc::detail::request_parameter<std::uint64_t>{"processKey"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"},
        snail::jsonrpc::detail::request_parameter<std::optional<progress_token>>{"workDoneToken"});

    const std::size_t& function_id() const
    {
        return std::get<0>(data_);
    }

    const std::uint64_t& process_key() const
    {
        return std::get<1>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<2>(data_);
    }

    const std::optional<progress_token>& work_done_token() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t,
        std::uint64_t,
        std::size_t,
        std::optional<progress_token>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<retrieve_line_info_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct set_sample_filters_request
{
    static constexpr std::string_view name = "setSampleFilters";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::optional<std::size_t>>{"minTime"},
        snail::jsonrpc::detail::request_parameter<std::optional<std::size_t>>{"maxTime"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::uint64_t>>{"excludedProcesses"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::uint64_t>>{"excludedThreads"},
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // In nanoseconds since session start.
    const std::optional<std::size_t>& min_time() const
    {
        return std::get<0>(data_);
    }
    // In nanoseconds since session start.
    const std::optional<std::size_t>& max_time() const
    {
        return std::get<1>(data_);
    }

    const std::vector<std::uint64_t>& excluded_processes() const
    {
        return std::get<2>(data_);
    }

    const std::vector<std::uint64_t>& excluded_threads() const
    {
        return std::get<3>(data_);
    }
    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<4>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::optional<std::size_t>,
        std::optional<std::size_t>,
        std::vector<std::uint64_t>,
        std::vector<std::uint64_t>,
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<set_sample_filters_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct impl_cancel_request_request
{
    static constexpr std::string_view name = "$/cancelRequest";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::variant<std::int64_t, std::string>>{"id"});

    const std::variant<std::int64_t, std::string>& id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::variant<std::int64_t, std::string>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<impl_cancel_request_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct close_document_request
{
    static constexpr std::string_view name = "closeDocument";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::size_t>{"documentId"});

    // The id of the document to perform the operation on.
    // This should be an id that resulted from a call to `readDocument`.
    const std::size_t& document_id() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::size_t>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<close_document_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct set_module_path_maps_request
{
    static constexpr std::string_view name = "setModulePathMaps";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::vector<std::tuple<std::string, std::string>>>{"simpleMaps"});

    const std::vector<std::tuple<std::string, std::string>>& simple_maps() const
    {
        return std::get<0>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::vector<std::tuple<std::string, std::string>>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<set_module_path_maps_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct set_pdb_symbol_find_options_request
{
    static constexpr std::string_view name = "setPdbSymbolFindOptions";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"searchDirs"},
        snail::jsonrpc::detail::request_parameter<std::optional<std::string>>{"symbolCacheDir"},
        snail::jsonrpc::detail::request_parameter<bool>{"noDefaultUrls"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"symbolServerUrls"});

    const std::vector<std::string>& search_dirs() const
    {
        return std::get<0>(data_);
    }

    const std::optional<std::string>& symbol_cache_dir() const
    {
        return std::get<1>(data_);
    }

    const bool& no_default_urls() const
    {
        return std::get<2>(data_);
    }

    const std::vector<std::string>& symbol_server_urls() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::vector<std::string>,
        std::optional<std::string>,
        bool,
        std::vector<std::string>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<set_pdb_symbol_find_options_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct set_dwarf_symbol_find_options_request
{
    static constexpr std::string_view name = "setDwarfSymbolFindOptions";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"searchDirs"},
        snail::jsonrpc::detail::request_parameter<std::optional<std::string>>{"debuginfodCacheDir"},
        snail::jsonrpc::detail::request_parameter<bool>{"noDefaultUrls"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"debuginfodUrls"});

    const std::vector<std::string>& search_dirs() const
    {
        return std::get<0>(data_);
    }

    const std::optional<std::string>& debuginfod_cache_dir() const
    {
        return std::get<1>(data_);
    }

    const bool& no_default_urls() const
    {
        return std::get<2>(data_);
    }

    const std::vector<std::string>& debuginfod_urls() const
    {
        return std::get<3>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        std::vector<std::string>,
        std::optional<std::string>,
        bool,
        std::vector<std::string>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<set_dwarf_symbol_find_options_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct set_module_filters_request
{
    static constexpr std::string_view name = "setModuleFilters";

    static constexpr auto parameters = std::tuple(
        snail::jsonrpc::detail::request_parameter<module_filter_mode>{"mode"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"exclude"},
        snail::jsonrpc::detail::request_parameter<std::vector<std::string>>{"include"});

    const module_filter_mode& mode() const
    {
        return std::get<0>(data_);
    }
    // Modules to exclude when `mode` is `AllButExcluded`. Supports wildcards (as in '*.exe').
    const std::vector<std::string>& exclude() const
    {
        return std::get<1>(data_);
    }
    // Modules to include when `mode` is `IncludedOnly`. Supports wildcards (as in '*.exe').
    const std::vector<std::string>& include() const
    {
        return std::get<2>(data_);
    }

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<
        module_filter_mode,
        std::vector<std::string>,
        std::vector<std::string>>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<set_module_filters_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail

struct exit_request
{
    static constexpr std::string_view name = "exit";

    static constexpr auto parameters = std::tuple();

    template<typename RequestType>
        requires snail::jsonrpc::detail::is_request_v<RequestType>
    friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data);

private:
    std::tuple<>
        data_;
};
namespace snail::jsonrpc::detail {
template<>
struct is_request<exit_request> : std::true_type
{};
} // namespace snail::jsonrpc::detail
