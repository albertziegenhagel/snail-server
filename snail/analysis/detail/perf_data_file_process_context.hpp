#pragma once

#include <cstdint>
#include <optional>
#include <set>

#include <snail/perf_data/build_id.hpp>
#include <snail/perf_data/dispatching_event_observer.hpp>

#include <snail/analysis/data/ids.hpp>

#include <snail/analysis/detail/id_at.hpp>
#include <snail/analysis/detail/module_map.hpp>
#include <snail/analysis/detail/process_history.hpp>
#include <snail/analysis/detail/stack_cache.hpp>

namespace snail::perf_data::parser {

struct fork_event_view;
struct comm_event_view;
struct mmap2_event_view;
struct sample_event;

} // namespace snail::perf_data::parser

namespace snail::analysis::detail {

class perf_data_file_process_context
{
public:
    using os_pid_t              = std::uint32_t;
    using os_tid_t              = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    using process_key = id_at<os_pid_t, timestamp_t>;
    using thread_key  = id_at<os_tid_t, timestamp_t>;

    struct sampled_process_info;
    struct sample_info;

    struct process_data
    {
        std::optional<std::string> name;

        std::optional<timestamp_t> end_time;

        std::optional<unique_process_id> unique_id;

        [[nodiscard]] friend bool operator==(const process_data& lhs, const process_data& rhs)
        {
            return lhs.name == rhs.name;
        }
    };
    struct thread_data
    {
        os_pid_t                   process_id;
        std::optional<std::string> name;

        std::optional<timestamp_t> end_time;

        std::optional<unique_thread_id> unique_id;

        [[nodiscard]] friend bool operator==(const thread_data& lhs, const thread_data& rhs)
        {
            return lhs.process_id == rhs.process_id &&
                   lhs.name == rhs.name;
        }
    };

    struct module_data
    {
        std::string                        filename;
        std::uint64_t                      page_offset;
        std::optional<perf_data::build_id> build_id;

        [[nodiscard]] friend bool operator==(const module_data& lhs, const module_data& rhs)
        {
            return lhs.filename == rhs.filename &&
                   lhs.page_offset == rhs.page_offset &&
                   lhs.build_id == rhs.build_id;
        }
    };

    using process_history = detail::history<os_pid_t, timestamp_t, process_data>;
    using thread_history  = detail::history<os_tid_t, timestamp_t, thread_data>;

    using process_info = process_history::entry;
    using thread_info  = thread_history::entry;

    explicit perf_data_file_process_context();

    ~perf_data_file_process_context();

    perf_data::dispatching_event_observer& observer();

    void finish();

    process_key id_to_key(unique_process_id id) const;
    thread_key  id_to_key(unique_thread_id id) const;

    const std::unordered_map<process_key, sampled_process_info>& sampled_processes() const;

    const process_history& get_processes() const;

    const thread_history& get_threads() const;

    const std::set<unique_thread_id>& get_process_threads(unique_process_id process_id) const;

    const module_map<module_data, timestamp_t>& get_modules(os_pid_t process_id) const;

    std::span<const sample_info> thread_samples(os_tid_t thread_id, timestamp_t start_time, std::optional<timestamp_t> end_time) const;

    const std::vector<instruction_pointer_t>& stack(std::size_t stack_index) const;

private:
    template<typename T>
    void register_event();

    void handle_event(const perf_data::parser::comm_event_view& event);
    void handle_event(const perf_data::parser::fork_event_view& event);
    void handle_event(const perf_data::parser::mmap2_event_view& event);
    void handle_event(const perf_data::parser::sample_event& event);

    perf_data::dispatching_event_observer observer_;

    process_history process_names;
    thread_history  thread_names;

    process_history processes;
    thread_history  threads;

    std::unordered_map<os_pid_t, std::set<thread_key>> threads_per_process_id_;

    std::unordered_map<unique_process_id, std::set<unique_thread_id>> threads_per_process_;

    std::unordered_map<unique_process_id, process_key> unique_process_id_to_key_;
    std::unordered_map<unique_thread_id, thread_key>   unique_thread_id_to_key_;

    std::unordered_map<os_pid_t, module_map<module_data, timestamp_t>> modules_per_process_id_;

    struct samples_storage
    {
        timestamp_t              first_sample_time = std::numeric_limits<timestamp_t>::max();
        timestamp_t              last_sample_time  = std::numeric_limits<timestamp_t>::min();
        std::vector<sample_info> samples;
    };

    std::unordered_map<os_tid_t, samples_storage> samples_per_thread_id_;

    std::unordered_map<process_key, sampled_process_info> sampled_processes_;

    stack_cache stacks;
};

struct perf_data_file_process_context::sampled_process_info
{
    os_pid_t process_id;

    timestamp_t process_timestamp;

    timestamp_t first_sample_time;
    timestamp_t last_sample_time;
};

struct perf_data_file_process_context::sample_info
{
    os_tid_t thread_id;

    timestamp_t timestamp;

    instruction_pointer_t instruction_pointer;

    std::size_t stack_index;
};

} // namespace snail::analysis::detail
