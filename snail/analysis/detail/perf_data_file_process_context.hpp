#pragma once

#include <cstdint>
#include <optional>
#include <set>

#include <snail/perf_data/dispatching_event_observer.hpp>

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
    using process_id_t          = std::uint32_t;
    using thread_id_t           = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    struct sample_info;

    struct process_data
    {
        std::optional<std::string> name;

        std::optional<timestamp_t> end_time;

        [[nodiscard]] friend bool operator==(const process_data& lhs, const process_data& rhs)
        {
            return lhs.name == rhs.name;
        }
    };
    struct thread_data
    {
        process_id_t               process_id;
        std::optional<std::string> name;

        std::optional<timestamp_t> end_time;

        [[nodiscard]] friend bool operator==(const thread_data& lhs, const thread_data& rhs)
        {
            return lhs.process_id == rhs.process_id &&
                   lhs.name == rhs.name;
        }
    };

    struct module_data
    {
        std::string   filename;
        std::uint64_t page_offset;

        [[nodiscard]] friend bool operator==(const module_data& lhs, const module_data& rhs)
        {
            return lhs.filename == rhs.filename &&
                   lhs.page_offset == rhs.page_offset;
        }
    };

    using process_history = detail::history<process_id_t, timestamp_t, process_data>;
    using thread_history  = detail::history<thread_id_t, timestamp_t, thread_data>;

    using process_info = process_history::entry;
    using thread_info  = thread_history::entry;

    explicit perf_data_file_process_context();

    ~perf_data_file_process_context();

    perf_data::dispatching_event_observer& observer();

    void finish();

    struct process_samples_storage
    {
        timestamp_t              first_sample_time = std::numeric_limits<timestamp_t>::max();
        timestamp_t              last_sample_time  = std::numeric_limits<timestamp_t>::min();
        std::vector<sample_info> samples;
    };

    const std::unordered_map<process_id_t, process_samples_storage>& get_samples_per_process() const;

    const process_history& get_processes() const;

    const thread_history& get_threads() const;

    const std::set<std::pair<thread_id_t, timestamp_t>>& get_process_threads(process_id_t process_id) const;

    const module_map<module_data>& get_modules(process_id_t process_id) const;

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

    std::unordered_map<process_id_t, std::set<std::pair<thread_id_t, timestamp_t>>> threads_per_process_;

    std::unordered_map<process_id_t, module_map<module_data>> modules_per_process;

    std::unordered_map<process_id_t, process_samples_storage> samples_per_process;

    stack_cache stacks;
};

struct perf_data_file_process_context::sample_info
{
    thread_id_t thread_id;

    timestamp_t timestamp;

    instruction_pointer_t instruction_pointer;

    std::size_t stack_index;
};

} // namespace snail::analysis::detail
