#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/data/types.hpp>

namespace snail::etl::parser {

struct system_config_v2_physical_disk_event_view;
struct system_config_v2_logical_disk_event_view;
struct process_v4_type_group1_event_view;
struct system_config_v2_physical_disk_event_view;
struct system_config_v2_physical_disk_event_view;
struct thread_v3_type_group1_event_view;
struct image_v2_load_event_view;
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
    struct process_info;
    struct thread_info;
    struct module_info;
    struct sample_info;

    explicit etl_file_process_context();

    ~etl_file_process_context();

    etl::dispatching_event_observer& observer();

    void resolve_nt_paths();

    const std::unordered_map<process_id_t, profiler_process_info>& profiler_processes() const;

    const process_info* try_get_process_at(process_id_t process_id, timestamp_t timestamp) const;

    const thread_info* try_get_thread_at(thread_id_t thread_id, timestamp_t timestamp) const;

    const module_info* try_get_module_at(process_id_t process_id, instruction_pointer_t address, timestamp_t timestamp) const;

    const std::vector<sample_info>& process_samples(process_id_t process_id) const;

    const std::vector<instruction_pointer_t>& stack(std::size_t stack_index) const;

private:
    template<typename T>
    void register_event();

    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v2_physical_disk_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::system_config_v2_logical_disk_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::process_v4_type_group1_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::thread_v3_type_group1_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::image_v2_load_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v2_sampled_profile_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::stackwalk_v2_stack_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event);

    process_info* try_get_process_at(process_id_t process_id, timestamp_t timestamp);
    thread_info*  try_get_thread_at(thread_id_t thread_id, timestamp_t timestamp);

    etl::dispatching_event_observer observer_;

    std::map<std::uint32_t, std::uint32_t>         number_of_partitions_per_disk;
    std::unordered_map<std::uint32_t, std::string> nt_partition_to_dos_volume_mapping;

    std::unordered_map<process_id_t, std::vector<process_info>> processes_by_id;
    std::unordered_map<thread_id_t, std::vector<thread_info>>   threads_by_id;

    std::unordered_map<process_id_t, profiler_process_info> profiler_processes_;

    std::unordered_map<process_id_t, std::vector<module_info>> modules_per_process;

    std::unordered_map<process_id_t, std::vector<sample_info>> samples_per_process;

    std::vector<std::vector<instruction_pointer_t>>           stacks;
    std::unordered_map<std::size_t, std::vector<std::size_t>> stack_map;
};

struct etl_file_process_context::process_info
{
    process_id_t process_id;

    timestamp_t first_event_time;

    std::string    image_filename;
    std::u16string command_line;
};

struct etl_file_process_context::thread_info
{
    thread_id_t thread_id;

    timestamp_t first_event_time;

    process_id_t process_id;
};

struct etl_file_process_context::profiler_process_info
{
    process_id_t process_id;

    timestamp_t start_timestamp;
};

struct etl_file_process_context::module_info
{
    std::size_t unique_id;

    timestamp_t load_timestamp;

    std::uint64_t image_base;
    std::uint64_t image_size;

    std::string file_name;
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
