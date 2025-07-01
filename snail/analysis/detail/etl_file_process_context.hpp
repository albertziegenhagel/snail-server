#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/analysis/data/ids.hpp>

#include <snail/analysis/detail/id_at.hpp>
#include <snail/analysis/detail/module_map.hpp>
#include <snail/analysis/detail/pdb_info.hpp>
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
struct thread_v4_context_switch_event_view;
struct image_v3_load_event_view;
struct perfinfo_v2_sampled_profile_event_view;
struct perfinfo_v2_pmc_counter_profile_event_view;
struct perfinfo_v3_sampled_profile_interval_event_view;
struct perfinfo_v2_pmc_counter_config_event_view;
struct stackwalk_v2_stack_event_view;
struct stackwalk_v2_key_event_view;
struct stackwalk_v2_type_group1_event_view;
struct image_id_v2_dbg_id_pdb_info_event_view;
struct vs_diagnostics_hub_target_profiling_started_event_view;
struct snail_profiler_profile_target_event_view;

} // namespace snail::etl::parser

namespace snail::analysis::detail {

class etl_file_process_context
{
public:
    using os_pid_t              = std::uint32_t;
    using os_tid_t              = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    using sample_source_id_t = std::uint16_t;

    using process_key = id_at<os_pid_t, timestamp_t>;
    using thread_key  = id_at<os_tid_t, timestamp_t>;

    struct profiler_process_info;
    struct sample_info;

    struct process_data
    {
        std::string    image_filename;
        std::u16string command_line;

        os_pid_t parent_id;

        std::optional<timestamp_t> end_time;

        std::optional<unique_process_id> unique_id;

        [[nodiscard]] friend bool operator==(const process_data& lhs, const process_data& rhs)
        {
            return lhs.image_filename == rhs.image_filename &&
                   lhs.command_line == rhs.command_line;
        }
    };
    struct thread_data
    {
        os_pid_t process_id;

        std::optional<timestamp_t> end_time;

        std::optional<unique_thread_id> unique_id;

        std::optional<std::u16string> name;

        std::size_t context_switches = 0;

        std::vector<std::size_t> pmc_counts;

        [[nodiscard]] friend bool operator==(const thread_data& lhs, const thread_data& rhs)
        {
            return lhs.process_id == rhs.process_id;
        }
    };

    struct module_data
    {
        std::string                     filename;
        std::uint32_t                   checksum;
        std::optional<detail::pdb_info> pdb_info;

        [[nodiscard]] friend bool operator==(const module_data& lhs, const module_data& rhs)
        {
            return lhs.filename == rhs.filename &&
                   lhs.checksum == rhs.checksum &&
                   lhs.pdb_info == rhs.pdb_info;
        }
    };

    using process_history = detail::history<os_pid_t, timestamp_t, process_data>;
    using thread_history  = detail::history<os_tid_t, timestamp_t, thread_data>;

    using process_info = process_history::entry;
    using thread_info  = thread_history::entry;

    explicit etl_file_process_context();

    ~etl_file_process_context();

    etl::dispatching_event_observer& observer();

    void finish();

    process_key id_to_key(unique_process_id id) const;
    thread_key  id_to_key(unique_thread_id id) const;

    const std::unordered_map<process_key, profiler_process_info>& profiler_processes() const;

    const process_history& get_processes() const;

    const thread_history& get_threads() const;

    const std::set<unique_thread_id>& get_process_threads(unique_process_id process_id) const;

    const module_map<module_data, timestamp_t>& get_modules(os_pid_t process_id) const;

    const std::unordered_map<sample_source_id_t, std::u16string>& sample_source_names() const;

    bool sample_source_has_stacks(sample_source_id_t pmc_source) const;

    std::span<const sample_info> thread_samples(os_tid_t                   thread_id,
                                                timestamp_t                start_time,
                                                std::optional<timestamp_t> end_time,
                                                sample_source_id_t         pmc_source) const;

    const std::vector<instruction_pointer_t>& stack(std::size_t stack_index) const;

    std::optional<std::u16string_view> computer_name() const;
    std::optional<std::uint16_t>       processor_architecture() const;
    std::optional<std::u16string_view> processor_name() const;
    std::optional<std::u16string_view> os_name() const;

    std::optional<std::u16string_view> pmc_name(std::size_t counter_index) const;

    bool has_context_switches() const;

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
    void handle_event(const etl::etl_file::header_data& file_header, const etl::any_group_trace_header& header_variant, const etl::parser::thread_v4_context_switch_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::image_v3_load_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v2_sampled_profile_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v2_pmc_counter_profile_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v3_sampled_profile_interval_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::perfinfo_v2_pmc_counter_config_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::stackwalk_v2_stack_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::stackwalk_v2_key_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::stackwalk_v2_type_group1_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::image_id_v2_dbg_id_pdb_info_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event);
    void handle_event(const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const etl::parser::snail_profiler_profile_target_event_view& event);

    etl::dispatching_event_observer observer_;

    std::map<std::uint32_t, std::uint32_t>         number_of_partitions_per_disk;
    std::unordered_map<std::uint32_t, std::string> nt_partition_to_dos_volume_mapping;

    std::unordered_map<std::string, std::string> nt_to_dos_path_map_;
    std::optional<std::string>                   system_root_;

    process_history processes;
    thread_history  threads;

    std::unordered_map<os_pid_t, std::set<thread_key>> threads_per_process_id_;

    std::unordered_map<process_key, profiler_process_info> profiler_processes_;

    struct pdb_info_storage
    {
        timestamp_t   event_timestamp;
        std::uint64_t image_base;
        pdb_info      info;
    };

    std::unordered_map<os_pid_t, std::vector<pdb_info_storage>>        modules_pdb_info_per_process_id_;
    std::unordered_map<os_pid_t, module_map<module_data, timestamp_t>> modules_per_process_id_;

    std::unordered_map<os_tid_t, std::unique_ptr<std::vector<sample_info>>> samples_per_thread_id_; // store as unique_ptr to allow stable references

    struct pmc_sample_storage
    {
        sample_source_id_t                        source;
        std::unique_ptr<std::vector<sample_info>> samples; // store as unique_ptr to allow stable references
    };

    std::unordered_map<os_tid_t, std::vector<pmc_sample_storage>> pmc_samples_per_thread_id_;

    struct sample_stack_ref
    {
        std::vector<sample_info>* samples;
        std::size_t               sample_index;
        bool                      is_kernel_stack;
        timestamp_t               stack_event_timestamp;
    };

    std::unordered_map<std::uint64_t, std::vector<sample_stack_ref>> cached_samples_per_stack_key_;

    std::unordered_map<sample_source_id_t, std::u16string> sample_source_names_;
    std::unordered_set<sample_source_id_t>                 sources_with_stacks_;

    struct context_switch_data
    {
        std::uint64_t exit_count  = 0;
        std::uint64_t enter_count = 0;

        timestamp_t first_time;
        timestamp_t last_time;

        struct pmc_counter_info
        {
            std::uint64_t                total_count      = 0;
            std::optional<std::uint64_t> prev_enter_count = std::nullopt;
        };

        std::vector<pmc_counter_info> pmc_counters_info;
    };

    bool has_context_switches_ = false;

    std::unordered_map<os_tid_t, context_switch_data> last_context_switch_data_per_thread_id_;

    std::unordered_map<unique_process_id, std::set<unique_thread_id>> threads_per_process_;

    std::unordered_map<unique_process_id, process_key> unique_process_id_to_key_;
    std::unordered_map<unique_thread_id, thread_key>   unique_thread_id_to_key_;

    std::vector<std::u16string> pmc_names_;

    stack_cache stacks;

    std::optional<std::u16string> computer_name_;
    std::optional<std::uint16_t>  processor_architecture_;
    std::optional<std::u16string> processor_name_;
    std::optional<std::u16string> os_name_;
};

struct etl_file_process_context::profiler_process_info
{
    os_pid_t process_id;

    timestamp_t start_timestamp;
};

struct etl_file_process_context::sample_info
{
    os_tid_t thread_id;

    timestamp_t timestamp;

    instruction_pointer_t instruction_pointer;

    std::optional<std::size_t> user_mode_stack;
    timestamp_t                user_timestamp;

    std::optional<std::size_t> kernel_mode_stack;
    timestamp_t                kernel_timestamp;
};

} // namespace snail::analysis::detail
