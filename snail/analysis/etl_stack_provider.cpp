
#include <snail/analysis/etl_stack_provider.hpp>

#include <ranges>

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

// struct etl_symbol_info : public symbol_info
// {
//     virtual std::string_view name() const override;
//     virtual bool             is_generic() const override;
// };

struct etl_stack_entry : public stack_entry
{
    virtual data::instruction_pointer_t instruction_pointer() const override { return ip; }
    virtual std::string_view            symbol_name() const override { return symbol_name_; }

    data::instruction_pointer_t ip;
    std::string_view            symbol_name_;
};

struct etl_sample_data : public sample_data
{
    virtual bool has_stack() const override
    {
        return user_stack != nullptr || kernel_stack != nullptr;
    }

    virtual common::generator<const stack_entry&> reversed_stack() const override
    {
        etl_stack_entry current_stack_entry;
        if(user_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*user_stack))
            {
                const auto [module, load_timestamp] = context->try_get_module_at(process_id, instruction_pointer, user_timestamp);

                const auto& symbol = (module == nullptr) ?
                                         resolver->make_generic_symbol(instruction_pointer) :
                                         resolver->resolve_symbol(detail::pdb_resolver::module_info{
                                                                      .image_filename = module->file_name,
                                                                      .image_base     = module->base,
                                                                      .process_id     = process_id,
                                                                      .load_timestamp = load_timestamp},
                                                                  instruction_pointer);

                current_stack_entry.ip           = instruction_pointer;
                current_stack_entry.symbol_name_ = symbol.name;
                co_yield current_stack_entry;
            }
        }
        if(kernel_stack != nullptr)
        {
            for(const auto instruction_pointer : std::views::reverse(*kernel_stack))
            {
                const auto [module, load_timestamp] = context->try_get_module_at(process_id, instruction_pointer, kernel_timestamp);

                const auto& symbol = (module == nullptr) ?
                                         resolver->make_generic_symbol(instruction_pointer) :
                                         resolver->resolve_symbol(detail::pdb_resolver::module_info{
                                                                      .image_filename = module->file_name,
                                                                      .image_base     = module->base,
                                                                      .process_id     = process_id,
                                                                      .load_timestamp = load_timestamp,
                                                                  },
                                                                  instruction_pointer);

                current_stack_entry.ip           = instruction_pointer;
                current_stack_entry.symbol_name_ = symbol.name;
                co_yield current_stack_entry;
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

struct etl_process_info : process_info
{
    virtual data::process_id_t process_id() const override { return process_id_; }
    virtual data::timestamp_t  start_time() const override { return start_time_; }

    virtual std::string_view image_name() const override { return image_name_; }

    data::process_id_t process_id_;
    data::timestamp_t  start_time_;
    std::string_view   image_name_;
};

} // namespace

etl_stack_provider::etl_stack_provider(const std::filesystem::path& file_path) :
    file_path_(file_path)
{}

etl_stack_provider::~etl_stack_provider() = default;

void etl_stack_provider::process()
{
    process_context_ = std::make_unique<detail::etl_file_process_context>();
    symbol_resolver_ = std::make_unique<detail::pdb_resolver>();

    etl::etl_file file(file_path_);
    file.process(process_context_->observer());

    process_context_->resolve_nt_paths();
}

common::generator<const process_info&> etl_stack_provider::processes() const
{
    if(process_context_ == nullptr) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    etl_process_info current_info;

    for(const auto& [process_id, profiler_process_info] : process_context.profiler_processes())
    {
        const auto* const process = process_context.try_get_process_at(process_id, profiler_process_info.start_timestamp);
        assert(process != nullptr);

        current_info.process_id_ = process_id;
        current_info.start_time_ = profiler_process_info.start_timestamp;
        current_info.image_name_ = process->image_filename;

        co_yield current_info;
    }
}

common::generator<const sample_data&> etl_stack_provider::samples(const process_info& process) const
{
    if(process_context_ == nullptr) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    std::unordered_map<
        detail::etl_file_process_context::thread_id_t,
        std::vector<const detail::etl_file_process_context::sample_info*>>
        remembered_kernel_samples;

    etl_sample_data current_sample_data;

    current_sample_data.process_id = process.process_id();
    current_sample_data.context    = process_context_.get();
    current_sample_data.resolver   = symbol_resolver_.get();

    // FIXME: retrieve this!
    const uint32_t pointer_size = 8;

    for(const auto& sample : process_context.process_samples(process.process_id()))
    {
        if(sample.user_mode_stack)
        {
            const auto& user_stack       = process_context.stack(*sample.user_mode_stack);
            const auto  starts_in_kernel = is_kernel_address(user_stack.back(), pointer_size);
            const auto  ends_in_kernel   = is_kernel_address(user_stack.front(), pointer_size);
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
