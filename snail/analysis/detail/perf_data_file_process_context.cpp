
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;
using namespace snail::analysis;
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
    // Assign names to processes & threads (and create missing ones)
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
                                                          .name      = entry.payload.name,
                                                          .end_time  = {},
                                                          .unique_id = {},
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
                                                        .unique_id  = {},
                                                    });
                threads_per_process_id_[entry.payload.process_id].emplace(id, entry.timestamp);
            }
        }
    }

    // Assign unique IDs to processes and threads.
    unique_process_id next_process_id{.key = 0x1'0000'0000};
    for(auto& [id, entries] : processes.all_entries())
    {
        process_info* prev = nullptr;
        for(auto& entry : entries)
        {
            entry.payload.unique_id = next_process_id;
            ++next_process_id.key;

            unique_process_id_to_key_[*entry.payload.unique_id] = process_key{id, entry.timestamp};

            if(prev != nullptr && prev->payload.end_time == std::nullopt)
            {
                prev->payload.end_time = entry.timestamp;
            }

            prev = &entry;
        }
    }
    unique_thread_id next_thread_id{.key = 0x2'0000'0000};
    for(auto& [id, entries] : threads.all_entries())
    {
        thread_info* prev = nullptr;
        for(auto& entry : entries)
        {
            entry.payload.unique_id = next_thread_id;
            ++next_thread_id.key;

            unique_thread_id_to_key_[*entry.payload.unique_id] = thread_key{id, entry.timestamp};

            if(prev != nullptr && prev->payload.end_time == std::nullopt)
            {
                prev->payload.end_time = entry.timestamp;
            }

            prev = &entry;
        }
    }

    // Build threads per proccess map
    for(const auto& [process_id, process_threads] : threads_per_process_id_)
    {
        for(const auto& thread_key : process_threads)
        {
            const auto* const process = processes.find_at(process_id, thread_key.time);
            if(process == nullptr) continue;

            const auto* const thread = threads.find_at(thread_key.id, thread_key.time);
            if(thread == nullptr) continue;

            threads_per_process_[*process->payload.unique_id].insert(*thread->payload.unique_id);
        }
    }

    // revert the event to source map, so that we can look-up the event descriptor
    // (if available) from the file metadata for each source later.
    for(const auto& [used_event_id, sample_source_id] : event_id_to_source_id_)
    {
        event_ids_per_sample_source_[sample_source_id].push_back(used_event_id);
    }
    assert(event_ids_per_sample_source_.size() == samples_per_source_and_thread_id_.size());

    for(const auto& [sample_id, samples_per_thread] : samples_per_source_and_thread_id_)
    {
        for(const auto& [thread_id, thread_entries] : threads.all_entries())
        {
            const auto iter = samples_per_thread.find(thread_id);
            if(iter == samples_per_thread.end()) continue;
            const auto& samples = iter->second;

            for(const auto& thread_entry : thread_entries)
            {
                if(samples.last_sample_time < thread_entry.timestamp ||
                   (thread_entry.payload.end_time && samples.first_sample_time > *thread_entry.payload.end_time)) continue;

                const auto* process = processes.find_at(thread_entry.payload.process_id, thread_entry.timestamp);
                if(process == nullptr) continue;

                const auto sampled_process_key = process_key{process->id, process->timestamp};

                auto proc_iter = sampled_processes_.find(sampled_process_key);
                if(proc_iter == sampled_processes_.end())
                {
                    sampled_processes_[sampled_process_key] = sampled_process_info{
                        .process_id        = process->id,
                        .process_timestamp = process->timestamp,
                    };
                }
            }
        }
    }
}

perf_data_file_process_context::process_key perf_data_file_process_context::id_to_key(unique_process_id id) const
{
    return unique_process_id_to_key_.at(id);
}

perf_data_file_process_context::thread_key perf_data_file_process_context::id_to_key(unique_thread_id id) const
{
    return unique_thread_id_to_key_.at(id);
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
    const auto pid = event.pid();
    const auto tid = event.tid();

    assert(event.sample_id().time);
    const auto time = *event.sample_id().time;

    if(pid == tid)
    {
        process_names.insert(pid, time, process_data{
                                            .name      = std::string(event.comm()),
                                            .end_time  = {},
                                            .unique_id = {},
                                        });
    }

    thread_names.insert(tid, time, thread_data{
                                       .process_id = pid,
                                       .name       = std::string(event.comm()),
                                       .end_time   = {},
                                       .unique_id  = {},
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
                                  .unique_id  = {},
                              });
    threads_per_process_id_[pid].emplace(tid, time);
}

void perf_data_file_process_context::handle_event(const perf_data::parser::mmap2_event_view& event)
{
    auto& process_modules = modules_per_process_id_[event.pid()];

    std::optional<perf_data::build_id> build_id;
    if(event.has_build_id())
    {
        build_id.emplace();
        build_id->size_ = event.build_id().size();
        std::ranges::copy(event.build_id(), build_id->buffer_.begin());
    }

    assert(event.sample_id().time);
    process_modules.insert(detail::module_info<module_data>{
                               .base    = event.addr(),
                               .size    = event.len(),
                               .payload = {
                                           .filename    = std::string(event.filename()),
                                           .page_offset = event.pgoff(),
                                           .build_id    = build_id}
    },
                           *event.sample_id().time);
}

void perf_data_file_process_context::handle_event(const perf_data::parser::sample_event& event)
{
    if(event.tid == std::nullopt ||
       event.time == std::nullopt) return;

    const auto unique_key            = reinterpret_cast<std::uintptr_t>(event.attributes);
    const auto [iter, is_new_source] = unique_sample_sources_.insert({unique_key, unique_sample_sources_.size()});

    const auto source_id = iter->second;
    if(is_new_source)
    {
        assert(!event_id_to_source_id_.contains(event.id) ||
               event_id_to_source_id_.at(event.id) == source_id);
        event_id_to_source_id_[event.id] = source_id;
    }

    auto& storage = samples_per_source_and_thread_id_[iter->second][*event.tid];

    storage.first_sample_time = std::min(storage.first_sample_time, *event.time);
    storage.last_sample_time  = std::max(storage.last_sample_time, *event.time);

    storage.samples.push_back(sample_info{
        .thread_id           = *event.tid,
        .timestamp           = *event.time,
        .instruction_pointer = event.ip,
        .stack_index         = event.ips ? std::make_optional(stacks.insert(*event.ips)) : std::nullopt});

    if(event.ips) sources_with_stacks_.insert(source_id);
}

const std::unordered_map<perf_data_file_process_context::process_key, perf_data_file_process_context::sampled_process_info>& perf_data_file_process_context::sampled_processes() const
{
    return sampled_processes_;
}

const std::unordered_map<perf_data_file_process_context::sample_source_id_t, std::vector<std::optional<std::uint64_t>>>& perf_data_file_process_context::event_ids_per_sample_source() const
{
    return event_ids_per_sample_source_;
}

bool perf_data_file_process_context::sample_source_has_stacks(sample_source_id_t source_id) const
{
    return sources_with_stacks_.contains(source_id);
}

std::span<const perf_data_file_process_context::sample_info> perf_data_file_process_context::thread_samples(os_tid_t thread_id, timestamp_t start_time, std::optional<timestamp_t> end_time, sample_source_id_t source_id) const
{
    auto iter = samples_per_source_and_thread_id_.find(source_id);
    if(iter == samples_per_source_and_thread_id_.end()) return {};

    auto iter2 = iter->second.find(thread_id);
    if(iter2 == iter->second.end()) return {};

    const auto& thread_samples = iter2->second.samples;

    const auto range_first_iter = std::ranges::lower_bound(thread_samples, start_time, std::less<>(), &sample_info::timestamp);
    if(range_first_iter == thread_samples.end()) return {};

    const auto range_end_iter = end_time != std::nullopt ?
                                    std::ranges::upper_bound(thread_samples, *end_time, std::less<>(), &sample_info::timestamp) :
                                    thread_samples.end();

    if(range_first_iter > range_end_iter) return {};

    return std::span(thread_samples).subspan(range_first_iter - thread_samples.begin(), range_end_iter - range_first_iter);
}

const perf_data_file_process_context::process_history& perf_data_file_process_context::get_processes() const
{
    return processes;
}

const perf_data_file_process_context::thread_history& perf_data_file_process_context::get_threads() const
{
    return threads;
}

const std::set<unique_thread_id>& perf_data_file_process_context::get_process_threads(unique_process_id process_id) const
{
    return threads_per_process_.at(process_id);
}

const module_map<perf_data_file_process_context::module_data, perf_data_file_process_context::timestamp_t>& perf_data_file_process_context::get_modules(os_pid_t process_id) const
{
    return modules_per_process_id_.at(process_id);
}

const std::vector<perf_data_file_process_context::instruction_pointer_t>& perf_data_file_process_context::stack(std::size_t stack_index) const
{
    return stacks.get(stack_index);
}
