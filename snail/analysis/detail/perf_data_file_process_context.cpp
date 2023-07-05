
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;
using namespace snail::analysis::detail;

perf_data_file_process_context::perf_data_file_process_context()
{
    register_event<perf_data::parser::comm_event_view>();
    register_event<perf_data::parser::fork_event_view>();
    register_event<perf_data::parser::mmap2_event_view>();
    register_event<perf_data::parser::sample_event>();
}

perf_data_file_process_context::~perf_data_file_process_context() = default;

perf_data::dispatching_event_observer& perf_data_file_process_context::observer()
{
    return observer_;
}

void perf_data_file_process_context::finish()
{
    for(const auto& [id, entries] : process_names.all_entries())
    {
        for(const auto& entry : entries)
        {
            auto* const process = processes.find_at(id, entry.timestamp, true);
            if(process != nullptr && process->payload.name == std::nullopt)
            {
                process->payload.name = entry.payload.name;
            }
            else
            {
                processes.insert(id, entry.timestamp, process_data{
                                                          .name     = entry.payload.name,
                                                          .end_time = {},
                                                      });
            }
        }
    }
    for(const auto& [id, entries] : thread_names.all_entries())
    {
        for(const auto& entry : entries)
        {
            auto* const thread = threads.find_at(id, entry.timestamp, true);
            if(thread != nullptr && thread->payload.name == std::nullopt)
            {
                thread->payload.name = entry.payload.name;
            }
            else
            {
                threads.insert(id, entry.timestamp, thread_data{
                                                        .process_id = entry.payload.process_id,
                                                        .name       = entry.payload.name,
                                                        .end_time   = {},
                                                    });
                threads_per_process_[entry.payload.process_id].emplace(id, entry.timestamp);
            }
        }
    }
}

template<typename T>
void perf_data_file_process_context::register_event()
{
    observer_.register_event<T>(
        [this](const T& event)
        {
            this->handle_event(event);
        });
}

void perf_data_file_process_context::handle_event(const perf_data::parser::comm_event_view& event)
{
    const auto pid  = event.pid();
    const auto tid  = event.tid();
    const auto time = *event.sample_id().time;
    if(pid == tid)
    {
        process_names.insert(pid, time, process_data{
                                            .name     = std::string(event.comm()),
                                            .end_time = {},
                                        });
    }

    thread_names.insert(pid, time, thread_data{
                                       .process_id = pid,
                                       .name       = std::string(event.comm()),
                                       .end_time   = {},
                                   });
}

void perf_data_file_process_context::handle_event(const perf_data::parser::fork_event_view& event)
{
    const auto pid  = event.pid();
    const auto tid  = event.tid();
    const auto time = event.time();

    if(pid == tid)
    {
        processes.insert(pid, time, process_data{});
    }

    threads.insert(tid, time, thread_data{
                                  .process_id = pid,
                                  .name       = {},
                                  .end_time   = {},
                              });
    threads_per_process_[pid].emplace(tid, time);
}

void perf_data_file_process_context::handle_event(const perf_data::parser::mmap2_event_view& event)
{
    auto& process_modules = modules_per_process[event.pid()];

    assert(event.sample_id().time);
    process_modules.insert(detail::module_info<module_data>{
                               .base    = event.addr(),
                               .size    = event.len(),
                               .payload = {
                                           .filename    = std::string(event.filename()),
                                           .page_offset = event.pgoff()}
    },
                           *event.sample_id().time);
}

void perf_data_file_process_context::handle_event(const perf_data::parser::sample_event& event)
{
    assert(event.pid);
    assert(event.tid);
    assert(event.ip);
    assert(event.ips);

    const auto stack_index = stacks.insert(*event.ips);

    auto& storage = samples_per_process[*event.pid];

    storage.first_sample_time = std::min(storage.first_sample_time, *event.time);
    storage.last_sample_time  = std::max(storage.last_sample_time, *event.time);

    storage.samples.push_back(sample_info{
        .thread_id           = *event.tid,
        .timestamp           = *event.time,
        .instruction_pointer = *event.ip,
        .stack_index         = stack_index});
}

const std::unordered_map<perf_data_file_process_context::process_id_t, perf_data_file_process_context::process_samples_storage>& perf_data_file_process_context::get_samples_per_process() const
{
    return samples_per_process;
}

const perf_data_file_process_context::process_history& perf_data_file_process_context::get_processes() const
{
    return processes;
}

const perf_data_file_process_context::thread_history& perf_data_file_process_context::get_threads() const
{
    return threads;
}

const std::set<std::pair<perf_data_file_process_context::thread_id_t, perf_data_file_process_context::timestamp_t>>& perf_data_file_process_context::get_process_threads(process_id_t process_id) const
{
    return threads_per_process_.at(process_id);
}

const module_map<perf_data_file_process_context::module_data>& perf_data_file_process_context::get_modules(process_id_t process_id) const
{
    return modules_per_process.at(process_id);
}

const std::vector<perf_data_file_process_context::instruction_pointer_t>& perf_data_file_process_context::stack(std::size_t stack_index) const
{
    return stacks.get(stack_index);
}
