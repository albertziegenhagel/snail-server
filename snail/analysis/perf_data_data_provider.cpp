#include <snail/analysis/perf_data_data_provider.hpp>

#include <format>
#include <numeric>
#include <queue>
#include <ranges>

#include <snail/common/cast.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/metadata.hpp>

#include <snail/analysis/detail/dwarf_resolver.hpp>
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

template<typename Duration>
Duration from_relative_timestamps(detail::perf_data_file_process_context::timestamp_t timestamp, detail::perf_data_file_process_context::timestamp_t start_timestamp)
{
    if(timestamp < start_timestamp) return Duration::zero();

    // NOTE: timestamps are in nanoseconds

    const auto relative_nanoseconds = timestamp - start_timestamp;
    return std::chrono::duration_cast<Duration>(std::chrono::nanoseconds(common::narrow_cast<std::chrono::nanoseconds::rep>(relative_nanoseconds)));
}

detail::perf_data_file_process_context::timestamp_t to_relative_timestamp(std::chrono::nanoseconds timestamp, detail::perf_data_file_process_context::timestamp_t start_timestamp)
{
    // NOTE: timestamps are in nanoseconds
    return start_timestamp + common::narrow_cast<detail::perf_data_file_process_context::timestamp_t>(timestamp.count());
}

struct time_span
{
    detail::perf_data_file_process_context::timestamp_t                start;
    std::optional<detail::perf_data_file_process_context::timestamp_t> end;
};

time_span get_filter_timespan(const detail::perf_data_file_process_context::thread_info& thread,
                              const sample_filter&                                       filter,
                              std::uint64_t                                              session_start_time)
{
    const auto filter_min_timestamp =
        filter.min_time == std::nullopt ?
            std::nullopt :
            std::make_optional(to_relative_timestamp(*filter.min_time, session_start_time));

    const auto filter_max_timestamp =
        filter.max_time == std::nullopt ?
            std::nullopt :
            std::make_optional(to_relative_timestamp(*filter.max_time, session_start_time));

    const auto sample_min_time =
        filter_min_timestamp ?
            std::max(*filter_min_timestamp, thread.timestamp) :
            thread.timestamp;

    const auto sample_max_time =
        filter_max_timestamp ?
            (thread.payload.end_time ?
                 std::min(*filter_max_timestamp, *thread.payload.end_time) :
                 *filter_max_timestamp) :
            thread.payload.end_time;

    return time_span{sample_min_time, sample_max_time};
}

const detail::perf_data_file_process_context::thread_info* get_thread_from_id(const detail::perf_data_file_process_context& process_context,
                                                                              unique_thread_id                              thread_id)
{
    const auto thread_key = process_context.id_to_key(thread_id);

    return process_context.get_threads().find_at(thread_key.id, thread_key.time);
}

struct perf_data_sample_data : public sample_data
{
    static constexpr std::string_view unkown_module_name = "[unknown]";

    std::optional<perf_data::build_id> try_get_module_build_id(const detail::perf_data_file_process_context::module_data& module) const
    {
        if(module.build_id) return module.build_id;
        if(build_id_map == nullptr) return std::nullopt;

        auto iter = build_id_map->find(module.filename);
        if(iter == build_id_map->end()) return std::nullopt;

        return iter->second;
    }

    stack_frame resolve_frame(std::uint64_t instruction_pointer) const
    {
        const auto [module, load_timestamp] = context->get_modules(process_id).find(instruction_pointer, timestamp_);

        const auto& symbol = (module == nullptr) ?
                                 resolver->make_generic_symbol(instruction_pointer) :
                                 resolver->resolve_symbol(detail::dwarf_resolver::module_info{
                                                              .image_filename = module->payload.filename,
                                                              .build_id       = try_get_module_build_id(module->payload),
                                                              .image_base     = module->base,
                                                              .page_offset    = module->payload.page_offset,
                                                              .process_id     = process_id,
                                                              .load_timestamp = load_timestamp},
                                                          instruction_pointer);

        return stack_frame{
            .symbol_name             = symbol.name,
            .module_name             = module == nullptr ? unkown_module_name : module->payload.filename,
            .file_path               = symbol.file_path,
            .function_line_number    = symbol.function_line_number,
            .instruction_line_number = symbol.instruction_line_number};
    }

    bool has_frame() const override
    {
        return instruction_pointer_ != std::nullopt;
    }

    bool has_stack() const override
    {
        return stack != nullptr;
    }

    common::generator<stack_frame> reversed_stack() const override
    {
        if(stack == nullptr) co_return;

        for(const auto instruction_pointer : std::views::reverse(*stack))
        {
            if(instruction_pointer >= std::to_underlying(perf_data::parser::sample_stack_context_marker::max))
            {
                // FIXME: handle context (how to handle that in reversed order??)
                continue;
            }

            co_yield resolve_frame(instruction_pointer);
        }
    }

    stack_frame frame() const override
    {
        return resolve_frame(*instruction_pointer_);
    }

    std::chrono::nanoseconds timestamp() const override
    {
        return from_relative_timestamps<std::chrono::nanoseconds>(timestamp_, session_start_time);
    }

    const detail::perf_data_file_process_context*                                     context;
    detail::dwarf_resolver*                                                           resolver;
    const std::unordered_map<std::string, perf_data::build_id>*                       build_id_map;
    detail::perf_data_file_process_context::os_pid_t                                  process_id;
    const std::vector<detail::perf_data_file_process_context::instruction_pointer_t>* stack;
    detail::perf_data_file_process_context::timestamp_t                               timestamp_;
    std::optional<std::uint64_t>                                                      instruction_pointer_;
    detail::perf_data_file_process_context::timestamp_t                               session_start_time;
};

struct next_sample_priority_info
{
    detail::perf_data_file_process_context::timestamp_t next_sample_time;
    std::size_t                                         thread_index;

    bool operator<(const next_sample_priority_info& other) const
    {
        // event with lowest timestamp has highest priority
        return next_sample_time > other.next_sample_time;
    }
};

struct thread_sample_data
{
    using sample_info = detail::perf_data_file_process_context::sample_info;

    std::span<const sample_info> samples;
    std::size_t                  next_sample_index;
};

} // namespace

perf_data_data_provider::perf_data_data_provider(dwarf_symbol_find_options find_options,
                                                 path_map                  module_path_map,
                                                 filter_options            module_filter)
{
    symbol_resolver_ = std::make_unique<detail::dwarf_resolver>(
        std::move(find_options),
        std::move(module_path_map),
        std::move(module_filter));
}

perf_data_data_provider::~perf_data_data_provider() = default;

void perf_data_data_provider::process(const std::filesystem::path&      file_path,
                                      const common::progress_listener*  progress_listener,
                                      const common::cancellation_token* cancellation_token)
{
    process_context_ = std::make_unique<detail::perf_data_file_process_context>();

    perf_data::perf_data_file file(file_path);
    file.process(process_context_->observer(),
                 progress_listener,
                 cancellation_token);

    process_context_->finish();

    const auto join = [](const std::vector<std::string>& strings) -> std::string
    {
#ifdef _MSC_VER // Missing compiler support for `std::views::join_with` in (at least) clang
        auto joined = strings | std::views::join_with(' ');
        return {std::ranges::begin(joined), std::ranges::end(joined)};
#else
        if(strings.empty()) return {};
        std::string result = strings.front();
        for(const auto& entry : strings | std::views::drop(1))
        {
            result.push_back(' ');
            result.append(entry);
        }
        return result;
#endif
    };

    using namespace std::chrono;

    struct time_range
    {
        detail::perf_data_file_process_context::timestamp_t start;
        detail::perf_data_file_process_context::timestamp_t end;
    };

    const auto& event_ids_per_sample_source = process_context_->event_ids_per_sample_source();

    std::unordered_map<detail::perf_data_file_process_context::sample_source_id_t, std::string> sample_source_names;

    sample_source_internal_ids_.clear();
    for(const auto& [internal_id, event_ids] : event_ids_per_sample_source)
    {
        std::optional<std::string> source_name;
        assert(!event_ids.empty());
        const auto event_id = event_ids.front(); // it should be sufficient to loop for one event id only.
        for(const auto& event_desc : file.metadata().event_desc)
        {
            // for an unset event, we just take the first event descriptor.
            if(event_id == std::nullopt)
            {
                source_name = event_desc.event_string;
                break;
            }

            if(std::ranges::find(event_desc.ids, *event_id) == event_desc.ids.end()) continue;

            source_name = event_desc.event_string;
            break;
        }

        if(source_name == std::nullopt)
        {
            // create a generic name
            source_name = std::format("unknown event {}", (sample_source_internal_ids_.size() + 1));
        }

        sample_source_internal_ids_.push_back(internal_id);

        sample_source_names[internal_id] = std::move(*source_name);
    }

    std::ranges::sort(sample_source_internal_ids_);

    sample_sources_.clear();
    std::vector<time_range> sample_source_times;
    for(const auto internal_id : sample_source_internal_ids_)
    {
        sample_sources_.push_back(sample_source_info{
            .id                    = sample_sources_.size(),
            .name                  = std::move(sample_source_names.at(internal_id)),
            .number_of_samples     = 0,
            .average_sampling_rate = 0.0,
            .has_stacks            = process_context_->sample_source_has_stacks(internal_id),
        });

        sample_source_times.push_back(time_range{
            .start = std::numeric_limits<detail::perf_data_file_process_context::timestamp_t>::max(),
            .end   = std::numeric_limits<detail::perf_data_file_process_context::timestamp_t>::min(),
        });
    }

    std::size_t total_sample_count = 0;
    std::size_t total_thread_count = 0;

    for(const auto& [process_key, sample_storage] : process_context_->sampled_processes())
    {
        const auto* const process = process_context_->get_processes().find_at(process_key.id, process_key.time);
        if(process == nullptr) continue;

        assert(process->payload.unique_id);
        const auto& threads = process_context_->get_process_threads(*process->payload.unique_id);
        total_thread_count += std::ranges::size(threads);

        for(const auto& thread_id : threads)
        {
            const auto* const thread = get_thread_from_id(*process_context_, thread_id);
            if(thread == nullptr) continue;

            for(sample_source_info::id_t source_id = 0; source_id < sample_sources_.size(); ++source_id)
            {
                const auto source_internal_id = sample_source_internal_ids_[source_id];
                const auto samples            = process_context_->thread_samples(thread->id, thread->timestamp, thread->payload.end_time, source_internal_id);

                auto& source_time_range = sample_source_times[source_id];

                if(!samples.empty())
                {
                    source_time_range.start = std::min(source_time_range.start, samples.front().timestamp);
                    source_time_range.end   = std::max(source_time_range.end, samples.back().timestamp);
                }

                sample_sources_[source_id].number_of_samples += std::ranges::size(samples);
                total_sample_count += std::ranges::size(samples);
            }
        }
    }

    auto start_timestamp = file.metadata().sample_time ? file.metadata().sample_time->start : nanoseconds::max();
    auto end_timestamp   = file.metadata().sample_time ? file.metadata().sample_time->end : nanoseconds::min();

    for(sample_source_info::id_t source_id = 0; source_id < sample_sources_.size(); ++source_id)
    {
        const auto& source_time_range = sample_source_times[source_id];

        if(source_time_range.start > source_time_range.end) continue; // skip empty

        auto& source_info = sample_sources_[source_id];

        const auto start_ns = nanoseconds(source_time_range.start);
        const auto end_ns   = nanoseconds(source_time_range.end);

        start_timestamp = std::min(start_timestamp, start_ns);
        end_timestamp   = std::max(end_timestamp, end_ns);

        const auto sampling_runtime = end_ns - start_ns;

        source_info.average_sampling_rate = sampling_runtime.count() == 0 ? 0.0 : ((double)source_info.number_of_samples / duration_cast<duration<double>>(sampling_runtime).count());
    }

    session_start_time_ = start_timestamp.count();
    session_end_time_   = end_timestamp.count();

    const auto runtime = start_timestamp < end_timestamp ? end_timestamp - start_timestamp : nanoseconds::zero();

    const auto file_modified_time = std::filesystem::last_write_time(file_path);

#ifdef _MSC_VER // FIXME: what is the correct test here?
    const auto date = time_point_cast<seconds>(clock_cast<system_clock>(file_modified_time));
#else
    const auto date = time_point_cast<seconds>(file_clock::to_sys(file_modified_time));
#endif

    if(file.metadata().build_ids)
    {
        build_id_map_ = file.metadata().build_ids;
    }

    session_info_ = analysis::session_info{
        .command_line        = file.metadata().cmdline ? join(*file.metadata().cmdline) : "[unknown]",
        .date                = date,
        .runtime             = runtime,
        .number_of_processes = process_context_->sampled_processes().size(),
        .number_of_threads   = total_thread_count,
        .number_of_samples   = total_sample_count,
    };

    system_info_ = analysis::system_info{
        .hostname             = file.metadata().hostname ? *file.metadata().hostname : "[unknown]",
        .platform             = file.metadata().os_release ? *file.metadata().os_release : "[unknown]",
        .architecture         = file.metadata().arch ? *file.metadata().arch : "[unknown]",
        .cpu_name             = file.metadata().cpu_desc ? *file.metadata().cpu_desc : "[unknown]",
        .number_of_processors = file.metadata().nr_cpus ? file.metadata().nr_cpus->nr_cpus_available : 0,
    };
}

const analysis::session_info& perf_data_data_provider::session_info() const
{
    assert(session_info_ != std::nullopt); // forget to call process()?
    return *session_info_;
}

const analysis::system_info& perf_data_data_provider::system_info() const
{
    assert(system_info_ != std::nullopt); // forget to call process()?
    return *system_info_;
}

common::generator<unique_process_id> perf_data_data_provider::sampling_processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    for(const auto& [process_key, storage] : process_context.sampled_processes())
    {
        const auto* const process = process_context_->get_processes().find_at(process_key.id, process_key.time);
        if(process == nullptr || process->payload.unique_id == std::nullopt) continue;

        co_yield unique_process_id(*process->payload.unique_id);
    }
}

analysis::process_info perf_data_data_provider::process_info(unique_process_id process_id) const
{
    assert(process_context_ != nullptr);

    const auto        process_key = process_context_->id_to_key(process_id);
    const auto* const process     = process_context_->get_processes().find_at(process_key.id, process_key.time);
    if(process == nullptr) throw std::runtime_error(std::format("Invalid process {} @{}", process_key.id, process_key.time));

    return analysis::process_info{
        .unique_id        = process_id,
        .os_id            = process->id,
        .name             = process->payload.name ? *process->payload.name : "[unknown]",
        .start_time       = from_relative_timestamps<std::chrono::nanoseconds>(process->timestamp, session_start_time_),
        .end_time         = process->payload.end_time ?
                                from_relative_timestamps<std::chrono::nanoseconds>(*process->payload.end_time, session_start_time_) :
                                from_relative_timestamps<std::chrono::nanoseconds>(session_end_time_, session_start_time_),
        .context_switches = {}, // TODO: get correct values
        .counters         = {}};
}

common::generator<analysis::thread_info> perf_data_data_provider::threads_info(unique_process_id process_id) const
{
    if(process_context_ == nullptr) co_return;

    const auto& process = process_info(process_id);

    for(const auto& thread_id : process_context_->get_process_threads(process_id))
    {
        const auto* const thread = get_thread_from_id(*process_context_, thread_id);
        if(thread == nullptr) continue;

        co_yield analysis::thread_info{
            .unique_id        = thread_id,
            .os_id            = thread->id,
            .name             = thread->payload.name,
            .start_time       = from_relative_timestamps<std::chrono::nanoseconds>(thread->timestamp, session_start_time_),
            .end_time         = thread->payload.end_time ?
                                    from_relative_timestamps<std::chrono::nanoseconds>(*thread->payload.end_time, session_start_time_) :
                                    process.end_time,
            .context_switches = {}, // TODO: get correct values
            .counters         = {}};
    }
}

const std::vector<sample_source_info>& perf_data_data_provider::sample_sources() const
{
    return sample_sources_;
}

common::generator<const sample_data&> perf_data_data_provider::samples(sample_source_info::id_t source_id,
                                                                       unique_process_id        process_id,
                                                                       const sample_filter&     filter) const
{
    if(process_context_ == nullptr) co_return;
    if(filter.excluded_processes.contains(process_id)) co_return;
    if(source_id >= sample_source_internal_ids_.size()) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    const auto process_key = process_context_->id_to_key(process_id);

    perf_data_sample_data current_sample_data;

    current_sample_data.process_id         = process_key.id;
    current_sample_data.context            = process_context_.get();
    current_sample_data.resolver           = symbol_resolver_.get();
    current_sample_data.build_id_map       = build_id_map_ ? &build_id_map_.value() : nullptr;
    current_sample_data.session_start_time = session_start_time_;

    const auto& threads = process_context.get_process_threads(process_id);

    std::priority_queue<next_sample_priority_info> sample_queue;

    std::vector<thread_sample_data> threads_samples;
    threads_samples.reserve(threads.size());

    const auto source_internal_id = sample_source_internal_ids_[source_id];

    for(const auto& thread_id : threads)
    {
        if(filter.excluded_threads.contains(thread_id)) continue;

        const auto* const thread = get_thread_from_id(*process_context_, thread_id);
        if(thread == nullptr) continue;

        const auto time_span = get_filter_timespan(*thread, filter, session_start_time_);

        const auto samples = process_context.thread_samples(thread->id, time_span.start, time_span.end, source_internal_id);

        if(samples.empty()) continue;

        sample_queue.push(next_sample_priority_info{
            .next_sample_time = samples.front().timestamp,
            .thread_index     = threads_samples.size()});

        threads_samples.push_back(thread_sample_data{
            .samples           = samples,
            .next_sample_index = 0});
    }

    while(!sample_queue.empty())
    {
        // Get the thread that has the earliest sample to extract next.
        const auto [_, thread_index] = sample_queue.top();
        sample_queue.pop();

        auto& thread_data = threads_samples[thread_index];

        // Extract samples for this thread as long as possible
        while(true)
        {
            const auto current_sample_index = thread_data.next_sample_index;
            ++thread_data.next_sample_index;

            const auto& sample = thread_data.samples[current_sample_index];

            current_sample_data.stack                = sample.stack_index ? &process_context.stack(*sample.stack_index) : nullptr;
            current_sample_data.timestamp_           = sample.timestamp;
            current_sample_data.instruction_pointer_ = sample.instruction_pointer;

            co_yield current_sample_data;

            // There are no more samples for this thread
            if(thread_data.next_sample_index >= thread_data.samples.size()) break;

            const auto next_sample_time = thread_data.samples[thread_data.next_sample_index].timestamp;

            if(!sample_queue.empty() && sample_queue.top().next_sample_time < next_sample_time)
            {
                // There is another thread which's next sample is earlier than the
                // next sample for the current thread. Hence, insert the current thread
                // sample into the priority queue and stop extracting samples for the current
                // thread.
                sample_queue.push(next_sample_priority_info{
                    .next_sample_time = next_sample_time,
                    .thread_index     = thread_index});
                break;
            }
        }
    }
}

std::size_t perf_data_data_provider::count_samples(sample_source_info::id_t source_id,
                                                   unique_process_id        process_id,
                                                   const sample_filter&     filter) const
{
    if(process_context_ == nullptr) return 0;
    if(filter.excluded_processes.contains(process_id)) return 0;
    if(source_id >= sample_source_internal_ids_.size()) return 0;

    std::size_t total_samples_count = 0;
    for(const auto& thread_id : process_context_->get_process_threads(process_id))
    {
        total_samples_count += count_samples(source_id, thread_id, filter);
    }
    return total_samples_count;
}

std::size_t perf_data_data_provider::count_samples(sample_source_info::id_t source_id,
                                                   unique_thread_id         thread_id,
                                                   const sample_filter&     filter) const
{
    if(process_context_ == nullptr) return 0;
    if(filter.excluded_threads.contains(thread_id)) return 0;
    if(source_id >= sample_source_internal_ids_.size()) return 0;

    assert(symbol_resolver_ != nullptr);

    const auto* const thread = get_thread_from_id(*process_context_, thread_id);
    if(thread == nullptr) return 0;

    const auto time_span = get_filter_timespan(*thread, filter, session_start_time_);

    return process_context_->thread_samples(thread->id, time_span.start, time_span.end, sample_source_internal_ids_[source_id]).size();
}
