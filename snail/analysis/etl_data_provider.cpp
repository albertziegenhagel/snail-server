
#include <snail/analysis/etl_data_provider.hpp>

#include <chrono>
#include <format>
#include <numeric>
#include <queue>
#include <ranges>

#include <utf8/cpp17.h>

#include <snail/common/cast.hpp>
#include <snail/common/string_compare.hpp>

#include <snail/etl/etl_file.hpp>

#include <snail/analysis/detail/etl_file_process_context.hpp>
#include <snail/analysis/detail/pdb_resolver.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

template<typename Rep, typename Ratio>
std::int64_t to_qpc_ticks(std::chrono::duration<Rep, Ratio> time, std::uint64_t qpc_frequency)
{
    // NOTE: Use gcd to reduce chance of overflow
    const auto gcd = std::gcd(Ratio::num * qpc_frequency, Ratio::den);
    return time.count() * ((Ratio::num * qpc_frequency) / gcd) / (Ratio::den / gcd);
}

template<typename Duration>
Duration from_qpc_ticks(std::int64_t ticks, std::uint64_t qpc_frequency)
{
    // NOTE: Use gcd to reduce chance of overflow
    const auto gcd = std::gcd(Duration::period::den, Duration::period::num * qpc_frequency);
    return Duration(ticks * (Duration::period::den / gcd) / ((Duration::period::num * qpc_frequency) / gcd));
}

template<typename Duration>
Duration from_relative_qpc_ticks(std::uint64_t ticks, std::uint64_t start, std::uint64_t qpc_frequency)
{
    if(ticks < start) return Duration::zero();
    const auto relative_ticks = ticks - start;
    return from_qpc_ticks<Duration>(common::narrow_cast<std::int64_t>(relative_ticks), qpc_frequency);
}

struct time_span
{
    detail::etl_file_process_context::timestamp_t                start;
    std::optional<detail::etl_file_process_context::timestamp_t> end;
};

time_span get_filter_timespan(const detail::etl_file_process_context::thread_info& thread,
                              const sample_filter&                                 filter,
                              std::uint64_t                                        session_start_qpc_ticks,
                              std::uint64_t                                        qpc_frequency)
{
    const auto filter_min_timestamp =
        filter.min_time == std::nullopt ?
            std::nullopt :
            std::make_optional(static_cast<detail::etl_file_process_context::timestamp_t>(
                session_start_qpc_ticks + std::max(to_qpc_ticks(*filter.min_time, qpc_frequency),
                                                   std::chrono::nanoseconds::rep(0))));

    const auto filter_max_timestamp =
        filter.max_time == std::nullopt ?
            std::nullopt :
            std::make_optional(static_cast<detail::etl_file_process_context::timestamp_t>(
                session_start_qpc_ticks + std::max(to_qpc_ticks(*filter.max_time, qpc_frequency),
                                                   std::chrono::nanoseconds::rep(0))));

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

const detail::etl_file_process_context::thread_info* get_thread_from_id(const detail::etl_file_process_context& process_context,
                                                                        unique_thread_id                        thread_id)
{
    const auto thread_key = process_context.id_to_key(thread_id);

    return process_context.get_threads().find_at(thread_key.id, thread_key.time);
}

struct etl_sample_data : public sample_data
{
    static constexpr detail::etl_file_process_context::os_pid_t kernel_process_id  = 0;
    static constexpr std::string_view                           unkown_module_name = "[unknown]";

    stack_frame resolve_frame(detail::etl_file_process_context::os_pid_t pid,
                              std::uint64_t                              instruction_pointer,
                              std::uint64_t                              timestamp) const
    {
        const auto [module, load_timestamp] = context->get_modules(pid).find(instruction_pointer, timestamp);

        const auto& symbol = (module == nullptr) ?
                                 resolver->make_generic_symbol(instruction_pointer) :
                                 resolver->resolve_symbol(detail::pdb_resolver::module_info{
                                                              .image_filename = module->payload.filename,
                                                              .image_base     = module->base,
                                                              .checksum       = module->payload.checksum,
                                                              .pdb_info       = module->payload.pdb_info,
                                                              .process_id     = pid,
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
        return true;
    }

    bool has_stack() const override
    {
        return user_stack != nullptr || kernel_stack != nullptr;
    }

    common::generator<stack_frame> reversed_stack() const override
    {
        if(user_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*user_stack))
            {
                co_yield resolve_frame(process_id, instruction_pointer, user_timestamp);
            }
        }
        if(kernel_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*kernel_stack))
            {
                co_yield resolve_frame(kernel_process_id, instruction_pointer, kernel_timestamp);
            }
        }
    }

    stack_frame frame() const override
    {
        return resolve_frame(process_id, instruction_pointer_, sample_timestamp);
    }

    std::chrono::nanoseconds timestamp() const override
    {
        return from_relative_qpc_ticks<std::chrono::nanoseconds>(sample_timestamp, session_start_qpc_ticks, qpc_frequency);
    }

    const std::vector<detail::etl_file_process_context::instruction_pointer_t>* user_stack;
    const std::vector<detail::etl_file_process_context::instruction_pointer_t>* kernel_stack;
    detail::etl_file_process_context::timestamp_t                               kernel_timestamp;
    detail::etl_file_process_context::timestamp_t                               user_timestamp;

    detail::etl_file_process_context::os_pid_t process_id;
    detail::pdb_resolver*                      resolver;
    detail::etl_file_process_context*          context;

    std::uint64_t instruction_pointer_;

    detail::etl_file_process_context::timestamp_t sample_timestamp;

    std::uint64_t session_start_qpc_ticks;
    std::uint64_t qpc_frequency;
};

std::string win_architecture_to_str(std::uint16_t arch)
{
    // see https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-processor
    switch(arch)
    {
    case 0: return "x86";
    case 1: return "MIPS";
    case 2: return "Alpha";
    case 3: return "PowerPC";
    case 5: return "ARM";
    case 6: return "ia64";
    case 9: return "x64";
    case 12: return "ARM64";
    }
    return "[unknown]";
}

struct next_sample_priority_info
{
    detail::etl_file_process_context::timestamp_t next_sample_time;
    std::size_t                                   thread_index;

    bool operator<(const next_sample_priority_info& other) const
    {
        // event with lowest timestamp has highest priority
        return next_sample_time > other.next_sample_time;
    }
};

struct thread_sample_data
{
    using sample_info = detail::etl_file_process_context::sample_info;

    std::span<const sample_info> samples;
    std::size_t                  next_sample_index;
};

} // namespace

etl_data_provider::etl_data_provider(pdb_symbol_find_options find_options,
                                     path_map                module_path_map,
                                     filter_options          module_filter)
{
    symbol_resolver_ = std::make_unique<detail::pdb_resolver>(
        std::move(find_options),
        std::move(module_path_map),
        std::move(module_filter));
}

etl_data_provider::~etl_data_provider() = default;

void etl_data_provider::process(const std::filesystem::path&      file_path,
                                const common::progress_listener*  progress_listener,
                                const common::cancellation_token* cancellation_token)
{
    process_context_ = std::make_unique<detail::etl_file_process_context>();

    etl::etl_file file(file_path);
    file.process(process_context_->observer(),
                 progress_listener,
                 cancellation_token);

    process_context_->finish();

    using namespace std::chrono;

    struct time_range
    {
        detail::etl_file_process_context::timestamp_t start;
        detail::etl_file_process_context::timestamp_t end;
    };

    const auto& sample_source_names = process_context_->sample_source_names();

    sample_source_internal_ids_.clear();
    for(const auto& [internal_id, name] : sample_source_names)
    {
        sample_source_internal_ids_.push_back(internal_id);
    }

    std::ranges::sort(sample_source_internal_ids_);

    sample_sources_.clear();
    std::vector<time_range> sample_source_times;
    for(const auto internal_id : sample_source_internal_ids_)
    {
        sample_sources_.push_back(sample_source_info{
            .id                    = sample_sources_.size(),
            .name                  = utf8::utf16to8(sample_source_names.at(internal_id)),
            .number_of_samples     = 0,
            .average_sampling_rate = 0.0,
            .has_stacks            = process_context_->sample_source_has_stacks(internal_id),
        });

        sample_source_times.push_back(time_range{
            .start = std::numeric_limits<detail::etl_file_process_context::timestamp_t>::max(),
            .end   = std::numeric_limits<detail::etl_file_process_context::timestamp_t>::min(),
        });
    }

    std::size_t total_sample_count = 0;
    std::size_t total_thread_count = 0;
    for(const auto& [process_key, profiler_process_info] : process_context_->profiler_processes())
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

    for(sample_source_info::id_t source_id = 0; source_id < sample_sources_.size(); ++source_id)
    {
        const auto& source_time_range = sample_source_times[source_id];

        if(source_time_range.start > source_time_range.end) continue; // skip empty

        auto& source_info = sample_sources_[source_id];

        const auto sampling_runtime = common::nt_duration(source_time_range.end) - common::nt_duration(source_time_range.start);

        source_info.average_sampling_rate = sampling_runtime.count() == 0 ? 0.0 : ((double)source_info.number_of_samples / duration_cast<duration<double>>(sampling_runtime).count());
    }
    pointer_size_ = file.header().pointer_size;

    const auto runtime = file.header().end_time - file.header().start_time;

    session_start_qpc_ticks_ = file.header().start_time_qpc_ticks;
    session_end_qpc_ticks_   = session_start_qpc_ticks_ + to_qpc_ticks(runtime, file.header().qpc_frequency);
    qpc_frequency_           = file.header().qpc_frequency;

    // This is a very rudimentary way to find the command line used to create the profile:
    // We assume that the profile has been collected via "vsdiagnostics.exe start"
    std::string command_line = "[unknown]";
    for(const auto& [id, infos] : process_context_->get_processes().all_entries())
    {
        if(common::ascii_iequals(infos.front().payload.image_filename, "vsdiagnostics.exe") &&
           infos.front().payload.command_line.contains(u" start "))
        {
            command_line = utf8::utf16to8(infos.front().payload.command_line);
            break;
        }
        if(common::ascii_iequals(infos.front().payload.image_filename, "xperf.exe") &&
           (infos.front().payload.command_line.contains(u" -start ") ||
            infos.front().payload.command_line.contains(u" -on ")))
        {
            command_line = utf8::utf16to8(infos.front().payload.command_line);
            break;
        }
    }

    session_info_ = analysis::session_info{
        .command_line        = std::move(command_line),
        .date                = time_point_cast<seconds>(file.header().start_time),
        .runtime             = std::chrono::duration_cast<std::chrono::nanoseconds>(runtime),
        .number_of_processes = process_context_->profiler_processes().size(),
        .number_of_threads   = total_thread_count,
        .number_of_samples   = total_sample_count,
    };

    system_info_ = analysis::system_info{
        .hostname             = process_context_->computer_name() ? utf8::utf16to8(*process_context_->computer_name()) : "[unknown]",
        .platform             = process_context_->os_name() ? utf8::utf16to8(*process_context_->os_name()) : "Windows",
        .architecture         = process_context_->processor_architecture() ? win_architecture_to_str(*process_context_->processor_architecture()) : "[unknown]",
        .cpu_name             = process_context_->processor_name() ? utf8::utf16to8(*process_context_->processor_name()) : "[unknown]",
        .number_of_processors = file.header().number_of_processors,
    };
}

const analysis::session_info& etl_data_provider::session_info() const
{
    assert(session_info_ != std::nullopt); // forgot to call process()?
    return *session_info_;
}

const analysis::system_info& etl_data_provider::system_info() const
{
    assert(system_info_ != std::nullopt); // forgot to call process()?
    return *system_info_;
}

common::generator<unique_process_id> etl_data_provider::sampling_processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    for(const auto& [process_key, profiler_process_info] : process_context.profiler_processes())
    {
        const auto* const process = process_context_->get_processes().find_at(process_key.id, process_key.time);
        if(process == nullptr || process->payload.unique_id == std::nullopt) continue;

        co_yield unique_process_id(*process->payload.unique_id);
    }
}

analysis::process_info etl_data_provider::process_info(unique_process_id process_id) const
{
    assert(process_context_ != nullptr);

    const auto        process_key = process_context_->id_to_key(process_id);
    const auto* const process     = process_context_->get_processes().find_at(process_key.id, process_key.time);
    if(process == nullptr) throw std::runtime_error(std::format("Invalid process {} @{}", process_key.id, process_key.time));

    // Sum context switches and PMC counters from all threads
    std::size_t                             context_switches = 0;
    std::vector<analysis::pmc_counter_info> counters;
    for(const auto& thread_id : process_context_->get_process_threads(process_id))
    {
        const auto* const thread = get_thread_from_id(*process_context_, thread_id);
        if(thread == nullptr) continue;

        context_switches += thread->payload.context_switches;

        if(counters.size() < thread->payload.pmc_counts.size())
        {
            counters.resize(thread->payload.pmc_counts.size());
        }
        for(std::size_t counter_index = 0; counter_index < thread->payload.pmc_counts.size(); ++counter_index)
        {
            counters[counter_index].count += thread->payload.pmc_counts[counter_index];
        }
    }
    for(std::size_t counter_index = 0; counter_index < counters.size(); ++counter_index)
    {
        counters[counter_index].name = process_context_->pmc_name(counter_index) ? std::make_optional(utf8::utf16to8(*process_context_->pmc_name(counter_index))) : std::nullopt;
    }

    return analysis::process_info{
        .unique_id        = process_id,
        .os_id            = process->id,
        .name             = process->payload.image_filename,
        .start_time       = from_relative_qpc_ticks<std::chrono::nanoseconds>(process->timestamp, session_start_qpc_ticks_, qpc_frequency_),
        .end_time         = process->payload.end_time ?
                                from_relative_qpc_ticks<std::chrono::nanoseconds>(*process->payload.end_time, session_start_qpc_ticks_, qpc_frequency_) :
                                from_relative_qpc_ticks<std::chrono::nanoseconds>(session_end_qpc_ticks_, session_start_qpc_ticks_, qpc_frequency_),
        .context_switches = process_context_->has_context_switches() ? context_switches : std::optional<std::size_t>(),
        .counters         = std::move(counters)};
}

common::generator<analysis::thread_info> etl_data_provider::threads_info(unique_process_id process_id) const
{
    if(process_context_ == nullptr) co_return;

    const auto& process = process_info(process_id);

    for(const auto& thread_id : process_context_->get_process_threads(process_id))
    {
        const auto* const thread = get_thread_from_id(*process_context_, thread_id);
        if(thread == nullptr) continue;

        std::vector<analysis::pmc_counter_info> counters;
        for(std::size_t counter_index = 0; counter_index < thread->payload.pmc_counts.size(); ++counter_index)
        {
            counters.push_back(analysis::pmc_counter_info{
                .name  = process_context_->pmc_name(counter_index) ? std::make_optional(utf8::utf16to8(*process_context_->pmc_name(counter_index))) : std::nullopt,
                .count = thread->payload.pmc_counts[counter_index]});
        }

        co_yield analysis::thread_info{
            .unique_id        = thread_id,
            .os_id            = thread->id,
            .name             = std::nullopt,
            .start_time       = from_relative_qpc_ticks<std::chrono::nanoseconds>(thread->timestamp, session_start_qpc_ticks_, qpc_frequency_),
            .end_time         = thread->payload.end_time ?
                                    from_relative_qpc_ticks<std::chrono::nanoseconds>(*thread->payload.end_time, session_start_qpc_ticks_, qpc_frequency_) :
                                    process.end_time,
            .context_switches = process_context_->has_context_switches() ? thread->payload.context_switches : std::optional<std::size_t>(),
            .counters         = std::move(counters)};
    }
}

const std::vector<sample_source_info>& etl_data_provider::sample_sources() const
{
    return sample_sources_;
}

common::generator<const sample_data&> etl_data_provider::samples(sample_source_info::id_t source_id,
                                                                 unique_process_id        process_id,
                                                                 const sample_filter&     filter) const
{
    if(process_context_ == nullptr) co_return;
    if(filter.excluded_processes.contains(process_id)) co_return;
    if(source_id >= sample_source_internal_ids_.size()) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    const auto process_key = process_context_->id_to_key(process_id);

    etl_sample_data current_sample_data;

    current_sample_data.process_id              = process_key.id;
    current_sample_data.context                 = process_context_.get();
    current_sample_data.resolver                = symbol_resolver_.get();
    current_sample_data.session_start_qpc_ticks = session_start_qpc_ticks_;
    current_sample_data.qpc_frequency           = qpc_frequency_;

    const auto& threads = process_context.get_process_threads(process_id);

    std::priority_queue<next_sample_priority_info> sample_queue;

    std::vector<thread_sample_data> threads_samples;
    threads_samples.reserve(threads.size());

    for(const auto& thread_id : threads)
    {
        if(filter.excluded_threads.contains(thread_id)) continue;

        const auto* const thread = get_thread_from_id(*process_context_, thread_id);
        if(thread == nullptr) continue;

        const auto time_span = get_filter_timespan(*thread, filter, session_start_qpc_ticks_, qpc_frequency_);

        const auto samples = process_context.thread_samples(thread->id, time_span.start, time_span.end, sample_source_internal_ids_[source_id]);

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

            current_sample_data.sample_timestamp     = sample.timestamp;
            current_sample_data.instruction_pointer_ = sample.instruction_pointer;

            if(sample.user_mode_stack)
            {
                current_sample_data.user_stack     = &process_context.stack(*sample.user_mode_stack);
                current_sample_data.user_timestamp = sample.user_timestamp;
            }
            else
            {
                current_sample_data.user_stack = nullptr;
            }

            if(sample.kernel_mode_stack)
            {
                current_sample_data.kernel_stack     = &process_context.stack(*sample.kernel_mode_stack);
                current_sample_data.kernel_timestamp = sample.kernel_timestamp;
            }
            else
            {
                current_sample_data.kernel_stack = nullptr;
            }

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

std::size_t etl_data_provider::count_samples(sample_source_info::id_t source_id,
                                             unique_process_id        process_id,
                                             const sample_filter&     filter) const
{
    if(process_context_ == nullptr) return 0;
    if(filter.excluded_processes.contains(process_id)) return 0;

    std::size_t total_samples_count = 0;
    for(const auto& thread_id : process_context_->get_process_threads(process_id))
    {
        total_samples_count += count_samples(source_id, thread_id, filter);
    }
    return total_samples_count;
}

std::size_t etl_data_provider::count_samples(sample_source_info::id_t source_id,
                                             unique_thread_id         thread_id,
                                             const sample_filter&     filter) const
{
    if(process_context_ == nullptr) return 0;
    if(filter.excluded_threads.contains(thread_id)) return 0;
    if(source_id >= sample_source_internal_ids_.size()) return 0;

    const auto* const thread = get_thread_from_id(*process_context_, thread_id);
    if(thread == nullptr) return 0;

    const auto time_span = get_filter_timespan(*thread, filter, session_start_qpc_ticks_, qpc_frequency_);

    return process_context_->thread_samples(thread->id, time_span.start, time_span.end, sample_source_internal_ids_[source_id]).size();
}
