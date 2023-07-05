#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/common/types.hpp>

#include <snail/analysis/detail/module_map.hpp>
#include <snail/analysis/detail/process_history.hpp>
#include <snail/analysis/detail/stack_cache.hpp>

namespace snail::etl::parser {

struct system_config_v3_cpu_event_view;
struct system_config_v2_physical_disk_event_view;
struct system_config_v2_logical_disk_event_view;
struct system_config_v5_pnp_event_view;
struct system_config_ex_v0_build_info_event_view;
struct system_config_ex_v0_system_paths_event_view;
struct system_config_ex_v0_volume_mapping_event_view;
struct process_v4_type_group1_event_view;
struct thread_v3_type_group1_event_view;
struct image_v3_load_event_view;
struct perfinfo_v2_sampled_profile_event_view;
struct stackwalk_v2_stack_event_view;
struct vs_diagnostics_hub_target_profiling_started_event_view;
struct vs_diagnostics_hub_target_profiling_stopped_event_view;

} // namespace snail::etl::parser

namespace snail::analysis::detail {

class etl_file_process_context
{
public:
    using process_id_t          = std::uint32_t;
    using thread_id_t           = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    struct profiler_process_info;
    struct sample_info;

    struct process_data
    {
        std::string    image_filename;
        std::u16string command_line;

        std::optional<timestamp_t> end_time;

        [[nodiscard]] friend bool operator==(const process_data& lhs, const process_data& rhs)
        {
            return lhs.image_filename == rhs.image_filename &&
                   lhs.command_line == rhs.command_line;
        }
    };
    struct thread_data
    {
        process_id_t process_id;

        std::optional<timestamp_t> end_time;

        [[nodiscard]] friend bool operator==(const thread_data& lhs, const thread_data& rhs)
        {
            return lhs.process_id == rhs.process_id;
        }
    };

    struct module_data
    {
        std::string filename;

        [[nodiscard]] friend bool operator==(const module_data& lhs, const module_data& rhs)
        {
            return lhs.filename == rhs.filename;
        }
    };

    using process_history = detail::history<process_id_t, timestamp_t, process_data>;
    using thread_history  = detail::history<thread_id_t, timestamp_t, thread_data>;

    using process_info = process_history::entry;
    using thread_info  = thread_history::entry;

    explicit etl_file_process_context();

    ~etl_file_process_context();

    etl::dispatching_event_observer& observer();

    void finish();

    const std::unordered_map<process_id_t, profiler_process_info>& profiler_processes() const;

    const process_history& get_processes() const;

    const thread_history& get_threads() const;

    const std::set<std::pair<thread_id_t, timestamp_t>>& get_process_threads(process_id_t process_id) const;

    const module_map<module_data>& get_modules(process_id_t process_id) const;

    const std::vector<sample_info>& process_samples(process_id_t process_id) const;

    const std::vector<instruction_pointer_t>& stack(std::size_t stack_index) const;

    std::optional<std::u16string_view> computer_name() const;
    std::optional<std::uint16_t>       processor_architecture() const;
    std::optional<std::u16string_view> processor_name() const;
    std::optional<std::u16string_view> os_name() const;

private:
    template<typename T>
    void register_event();

    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v3_cpu_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v2_physical_disk_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v2_logical_disk_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v5_pnp_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_ex_v0_build_info_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_ex_v0_system_paths_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_ex_v0_volume_mapping_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::process_v4_type_group1_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::thread_v3_type_group1_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::image_v3_load_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v2_sampled_profile_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::stackwalk_v2_stack_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event);

    etl::dispatching_event_observer observer_;

    std::map<std::uint32_t, std::uint32_t>         number_of_partitions_per_disk;
    std::unordered_map<std::uint32_t, std::string> nt_partition_to_dos_volume_mapping;

    std::unordered_map<std::string, std::string> nt_to_dos_path_map_;
    std::optional<std::string>                   system_root_;

    process_history processes;
    thread_history  threads;

    std::unordered_map<process_id_t, std::set<std::pair<thread_id_t, timestamp_t>>> threads_per_process_;

    std::unordered_map<process_id_t, profiler_process_info> profiler_processes_;

    std::unordered_map<process_id_t, module_map<module_data>> modules_per_process;

    std::unordered_map<process_id_t, std::vector<sample_info>> samples_per_process;

    stack_cache stacks;

    std::optional<std::u16string> computer_name_;
    std::optional<std::uint16_t>  processor_architecture_;
    std::optional<std::u16string> processor_name_;
    std::optional<std::u16string> os_name_;
};

struct etl_file_process_context::profiler_process_info
{
    process_id_t process_id;

    timestamp_t start_timestamp;
};

struct etl_file_process_context::sample_info
{
    thread_id_t thread_id;

    timestamp_t timestamp;

    instruction_pointer_t instruction_pointer;

    std::optional<std::size_t> user_mode_stack;
    std::optional<std::size_t> kernel_mode_stack;
};

} // namespace snail::analysis::detail
