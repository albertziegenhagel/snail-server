
#include <snail/analysis/etl_data_provider.hpp>

#include <chrono>
#include <format>
#include <numeric>
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

bool is_kernel_address(std::uint64_t address, std::uint32_t pointer_size)
{
    // See https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/user-space-and-system-space
    return pointer_size == 4 ?
               address >= 0x80000000 :        // 32bit
               address >= 0x0000800000000000; // 64bit
}

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

struct etl_sample_data : public sample_data
{
    [[nodiscard]] bool has_stack() const override
    {
        return user_stack != nullptr || kernel_stack != nullptr;
    }

    common::generator<stack_frame> reversed_stack() const override
    {
        static const std::string unkown_module_name = "[unknown]";

        if(user_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*user_stack))
            {
                const auto [module, load_timestamp] = context->get_modules(process_id).find(instruction_pointer, user_timestamp);

                const auto& symbol = (module == nullptr) ?
                                         resolver->make_generic_symbol(instruction_pointer) :
                                         resolver->resolve_symbol(detail::pdb_resolver::module_info{
                                                                      .image_filename = module->file_name,
                                                                      .image_base     = module->base,
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
        if(kernel_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*kernel_stack))
            {
                const auto [module, load_timestamp] = context->get_modules(process_id).find(instruction_pointer, kernel_timestamp);

                const auto& symbol = (module == nullptr) ?
                                         resolver->make_generic_symbol(instruction_pointer) :
                                         resolver->resolve_symbol(detail::pdb_resolver::module_info{
                                                                      .image_filename = module->file_name,
                                                                      .image_base     = module->base,
                                                                      .process_id     = process_id,
                                                                      .load_timestamp = load_timestamp,
                                                                  },
                                                                  instruction_pointer);

                co_yield stack_frame{
                    .symbol_name             = symbol.name,
                    .module_name             = module == nullptr ? unkown_module_name : module->file_name,
                    .file_path               = symbol.file_path,
                    .function_line_number    = symbol.function_line_number,
                    .instruction_line_number = symbol.instruction_line_number};
            }
        }
    }

    const std::vector<detail::etl_file_process_context::instruction_pointer_t>* user_stack;
    const std::vector<detail::etl_file_process_context::instruction_pointer_t>* kernel_stack;
    detail::etl_file_process_context::timestamp_t                               kernel_timestamp;
    detail::etl_file_process_context::timestamp_t                               user_timestamp;

    detail::etl_file_process_context::process_id_t process_id;
    detail::pdb_resolver*                          resolver;
    detail::etl_file_process_context*              context;
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

} // namespace

etl_data_provider::~etl_data_provider() = default;

void etl_data_provider::process(const std::filesystem::path& file_path)
{
    process_context_ = std::make_unique<detail::etl_file_process_context>();
    symbol_resolver_ = std::make_unique<detail::pdb_resolver>();

    etl::etl_file file(file_path);
    file.process(process_context_->observer());

    process_context_->finish();

    using namespace std::chrono;

    std::size_t total_sample_count = 0;
    std::size_t total_thread_count = 0;
    for(const auto& [process_id, profiler_process_info] : process_context_->profiler_processes())
    {
        const auto& samples = process_context_->process_samples(process_id);
        total_sample_count += samples.size();
        total_thread_count += std::ranges::size(process_context_->get_process_threads(process_id));
    }

    const auto runtime = file.header().end_time - file.header().start_time;

    session_start_qpc_ticks_ = file.header().start_time_qpc_ticks;
    session_end_qpc_ticks_   = session_start_qpc_ticks_ + to_qpc_ticks(runtime, file.header().qpc_frequency);
    qpc_frequency_           = file.header().qpc_frequency;

    const auto average_sampling_rate = runtime.count() == 0 ? 0.0 : ((double)total_sample_count / duration_cast<duration<double>>(runtime).count());

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
    }

    session_info_ = analysis::session_info{
        .command_line          = std::move(command_line),
        .date                  = time_point_cast<seconds>(file.header().start_time),
        .runtime               = std::chrono::duration_cast<std::chrono::nanoseconds>(runtime),
        .number_of_processes   = process_context_->profiler_processes().size(),
        .number_of_threads     = total_thread_count,
        .number_of_samples     = total_sample_count,
        .average_sampling_rate = average_sampling_rate};

    system_info_ = analysis::system_info{
        .hostname             = process_context_->computer_name() ? utf8::utf16to8(*process_context_->computer_name()) : "[unknown]",
        .platform             = "Windows",
        .architecture         = process_context_->processor_architecture() ? win_architecture_to_str(*process_context_->processor_architecture()) : "[unknown]",
        .cpu_name             = process_context_->processor_name() ? utf8::utf16to8(*process_context_->processor_name()) : "[unknown]",
        .number_of_processors = file.header().number_of_processors,
    };
}

const analysis::session_info& etl_data_provider::session_info() const
{
    assert(session_info_ != std::nullopt); // forget to call process()?
    return *session_info_;
}

const analysis::system_info& etl_data_provider::system_info() const
{
    assert(system_info_ != std::nullopt); // forget to call process()?
    return *system_info_;
}

common::generator<common::process_id_t> etl_data_provider::sampling_processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    for(const auto& [process_id, profiler_process_info] : process_context.profiler_processes())
    {
        co_yield common::process_id_t(process_id);
    }
}

analysis::process_info etl_data_provider::process_info(common::process_id_t process_id) const
{
    assert(process_context_ != nullptr);

    const auto& profiler_process_info = process_context_->profiler_processes().at(process_id);

    const auto* const process = process_context_->get_processes().find_at(process_id, profiler_process_info.start_timestamp);
    if(process == nullptr) throw std::runtime_error(std::format("Invalid process {}", process_id));

    return analysis::process_info{
        .id         = process_id,
        .name       = process->payload.image_filename,
        .start_time = from_relative_qpc_ticks<std::chrono::nanoseconds>(process->timestamp, session_start_qpc_ticks_, qpc_frequency_),
        .end_time   = process->payload.end_time ?
                          from_relative_qpc_ticks<std::chrono::nanoseconds>(*process->payload.end_time, session_start_qpc_ticks_, qpc_frequency_) :
                          from_relative_qpc_ticks<std::chrono::nanoseconds>(session_end_qpc_ticks_, session_start_qpc_ticks_, qpc_frequency_)};
}

common::generator<analysis::thread_info> etl_data_provider::threads_info(common::process_id_t process_id) const
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
            .name       = std::nullopt,
            .start_time = from_relative_qpc_ticks<std::chrono::nanoseconds>(thread->timestamp, session_start_qpc_ticks_, qpc_frequency_),
            .end_time   = thread->payload.end_time ?
                              from_relative_qpc_ticks<std::chrono::nanoseconds>(*thread->payload.end_time, session_start_qpc_ticks_, qpc_frequency_) :
                              process.end_time};
    }
}

common::generator<const sample_data&> etl_data_provider::samples(common::process_id_t process_id) const
{
    if(process_context_ == nullptr) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    std::unordered_map<
        detail::etl_file_process_context::thread_id_t,
        std::vector<const detail::etl_file_process_context::sample_info*>>
        remembered_kernel_samples;

    etl_sample_data current_sample_data;

    current_sample_data.process_id = process_id;
    current_sample_data.context    = process_context_.get();
    current_sample_data.resolver   = symbol_resolver_.get();

    // FIXME: retrieve this!
    const uint32_t pointer_size = 8;

    for(const auto& sample : process_context.process_samples(process_id))
    {
        if(sample.user_mode_stack)
        {
            const auto&                 user_stack       = process_context.stack(*sample.user_mode_stack);
            [[maybe_unused]] const auto starts_in_kernel = is_kernel_address(user_stack.back(), pointer_size);
            const auto                  ends_in_kernel   = is_kernel_address(user_stack.front(), pointer_size);
            assert(!starts_in_kernel);

            if(ends_in_kernel)
            {
                current_sample_data.user_stack     = &user_stack;
                current_sample_data.user_timestamp = sample.timestamp;
                current_sample_data.kernel_stack   = nullptr;
                co_yield current_sample_data;
            }
            else
            {
                auto& remembered_samples = remembered_kernel_samples[sample.thread_id];
                for(const auto* const remembered_sample : remembered_samples)
                {
                    assert(remembered_sample->kernel_mode_stack);
                    const auto& kernel_stack = process_context.stack(*remembered_sample->kernel_mode_stack);

                    current_sample_data.user_stack       = &user_stack;
                    current_sample_data.user_timestamp   = sample.timestamp;
                    current_sample_data.kernel_stack     = &kernel_stack;
                    current_sample_data.kernel_timestamp = remembered_sample->timestamp;
                    co_yield current_sample_data;
                }
                remembered_samples.clear();

                if(sample.kernel_mode_stack)
                {
                    const auto& kernel_stack = process_context.stack(*sample.kernel_mode_stack);

                    current_sample_data.user_stack       = &user_stack;
                    current_sample_data.user_timestamp   = sample.timestamp;
                    current_sample_data.kernel_stack     = &kernel_stack;
                    current_sample_data.kernel_timestamp = sample.timestamp;
                    co_yield current_sample_data;
                }
                else
                {
                    current_sample_data.user_stack     = &user_stack;
                    current_sample_data.user_timestamp = sample.timestamp;
                    current_sample_data.kernel_stack   = nullptr;
                    co_yield current_sample_data;
                }
            }
        }
        else if(sample.kernel_mode_stack)
        {
            remembered_kernel_samples[sample.thread_id].push_back(&sample);
        }
        else
        {
            current_sample_data.user_stack   = nullptr;
            current_sample_data.kernel_stack = nullptr;
            co_yield current_sample_data;
        }
    }

    for(const auto& [thread_id, remembered_samples] : remembered_kernel_samples)
    {
        for(const auto* const remembered_sample : remembered_samples)
        {
            assert(remembered_sample->kernel_mode_stack);
            const auto& kernel_stack = process_context.stack(*remembered_sample->kernel_mode_stack);

            current_sample_data.user_stack       = nullptr;
            current_sample_data.kernel_stack     = &kernel_stack;
            current_sample_data.kernel_timestamp = remembered_sample->timestamp;
            co_yield current_sample_data;
        }
    }
}
