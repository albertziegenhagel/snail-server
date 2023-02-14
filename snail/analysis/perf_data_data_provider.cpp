#include <snail/analysis/perf_data_data_provider.hpp>

#include <format>
#include <numeric>
#include <ranges>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/detail/metadata.hpp>

#include <snail/analysis/detail/dwarf_resolver.hpp>
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

struct perf_data_sample_data : public sample_data
{
    virtual bool has_stack() const override
    {
        return true;
    }

    virtual common::generator<stack_frame> reversed_stack() const override
    {
        static const std::string unkown_module_name = "[unknown]";

        for(const auto instruction_pointer : std::views::reverse(*stack))
        {
            if(instruction_pointer >= std::to_underlying(perf_data::parser::sample_stack_context_marker::max))
            {
                // FIXME: handle context (how to handle that in reversed order??)
                continue;
            }

            const auto [module, load_timestamp] = context->get_modules(process_id).find(instruction_pointer, timestamp);

            const auto& symbol = (module == nullptr) ?
                                     resolver->make_generic_symbol(instruction_pointer) :
                                     resolver->resolve_symbol(detail::dwarf_resolver::module_info{
                                                                  .image_filename = module->file_name,
                                                                  .image_base     = module->base,
                                                                  .page_offset    = module->page_offset,
                                                                  .process_id     = process_id,
                                                                  .load_timestamp = load_timestamp},
                                                              instruction_pointer);

            co_yield stack_frame{
                .symbol_name             = symbol.name,
                .module_name             = module == nullptr ? unkown_module_name : module->file_name,
                .file_path               = symbol.file_path,
                .function_line_number    = symbol.function_line_number,
                .instruction_line_number = symbol.instruction_line_number};
        }
    }

    const detail::perf_data_file_process_context*     context;
    detail::dwarf_resolver*                           resolver;
    common::process_id_t                              process_id;
    const std::vector<common::instruction_pointer_t>* stack;
    common::timestamp_t                               timestamp;
};

} // namespace

perf_data_data_provider::~perf_data_data_provider() = default;

void perf_data_data_provider::process(const std::filesystem::path& file_path)
{
    process_context_ = std::make_unique<detail::perf_data_file_process_context>();
    symbol_resolver_ = std::make_unique<detail::dwarf_resolver>();

    perf_data::perf_data_file file(file_path);
    file.process(process_context_->observer());

    process_context_->finish();

    const auto join = [](const std::vector<std::string>& strings)
    {
        auto joined = strings | std::views::join_with(' ');
        return std::string(std::ranges::begin(joined), std::ranges::end(joined));
    };

    using namespace std::chrono;

    std::size_t total_sample_count = 0;
    std::size_t total_thread_count = 0;
    auto        start_timestamp    = file.metadata().sample_time ? file.metadata().sample_time->start : nanoseconds::max();
    auto        end_timestamp      = file.metadata().sample_time ? file.metadata().sample_time->end : nanoseconds::min();

    for(const auto& [process_id, sample_storage] : process_context_->get_samples_per_process())
    {
        total_sample_count += sample_storage.samples.size();
        start_timestamp = std::min(start_timestamp, nanoseconds(sample_storage.first_sample_time));
        end_timestamp   = std::max(end_timestamp, nanoseconds(sample_storage.last_sample_time));
        total_thread_count += std::ranges::size(process_context_->get_process_threads(process_id));
    }

    session_start_time_ = start_timestamp.count();
    session_end_time_   = end_timestamp.count();

    const auto runtime = start_timestamp < end_timestamp ? end_timestamp - start_timestamp : nanoseconds(0);

    const auto average_sampling_rate = runtime.count() == 0 ? 0.0 : (total_sample_count / duration_cast<duration<double>>(runtime).count());

    session_info_ = analysis::session_info{
        .command_line          = file.metadata().cmdline ? join(*file.metadata().cmdline) : "[unknown]",
        .date                  = time_point_cast<seconds>(file_clock::to_utc(std::filesystem::last_write_time(file_path))),
        .runtime               = runtime,
        .number_of_processes   = process_context_->get_samples_per_process().size(),
        .number_of_threads     = total_thread_count,
        .number_of_samples     = total_sample_count,
        .average_sampling_rate = average_sampling_rate};

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

common::generator<common::process_id_t> perf_data_data_provider::sampling_processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    for(const auto& [process_id, storage] : process_context.get_samples_per_process())
    {
        co_yield common::process_id_t(process_id);
    }
}

analysis::process_info perf_data_data_provider::process_info(common::process_id_t process_id) const
{
    assert(process_context_ != nullptr);

    const auto& sample_storage = process_context_->get_samples_per_process().at(process_id);

    const auto* const process = process_context_->get_processes().find_at(process_id, sample_storage.first_sample_time);
    if(process == nullptr) throw std::runtime_error(std::format("Invalid process {}", process_id));

    return analysis::process_info{
        .id         = process_id,
        .name       = process->payload.name ? *process->payload.name : "[unknown]",
        .start_time = process->timestamp >= session_start_time_ ? process->timestamp - session_start_time_ : 0,
        .end_time   = process->payload.end_time ?
                          (*process->payload.end_time >= session_start_time_ ? *process->payload.end_time - session_start_time_ : 0) :
                          session_end_time_};
}

common::generator<analysis::thread_info> perf_data_data_provider::threads_info(common::process_id_t process_id) const
{
    if(process_context_ == nullptr) co_return;

    const auto& process = process_info(process_id);

    for(const auto& [thread_id, time] : process_context_->get_process_threads(process_id))
    {
        const auto* const thread = process_context_->get_threads().find_at(thread_id, time);

        assert(thread != nullptr);
        if(thread == nullptr) continue; // fault tolerance in release mode

        co_yield analysis::thread_info{
            .id         = thread->id,
            .name       = thread->payload.name,
            .start_time = thread->timestamp >= session_start_time_ ? thread->timestamp - session_start_time_ : 0,
            .end_time   = thread->payload.end_time ?
                              (*thread->payload.end_time >= session_start_time_ ? *thread->payload.end_time - session_start_time_ : 0) :
                              process.end_time};
    }
}

common::generator<const sample_data&> perf_data_data_provider::samples(common::process_id_t process_id) const
{
    if(process_context_ == nullptr) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    const auto& sample_storage = process_context.get_samples_per_process().at(process_id);

    perf_data_sample_data current_sample_data;
    current_sample_data.process_id = process_id;
    current_sample_data.context    = process_context_.get();
    current_sample_data.resolver   = symbol_resolver_.get();

    for(const auto& sample : sample_storage.samples)
    {
        current_sample_data.stack     = &process_context.stack(sample.stack_index);
        current_sample_data.timestamp = sample.timestamp;
        co_yield current_sample_data;
    }
}
