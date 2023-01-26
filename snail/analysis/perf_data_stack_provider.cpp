#include <snail/analysis/perf_data_stack_provider.hpp>

#include <ranges>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/analysis/detail/dwarf_resolver.hpp>
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

struct perf_data_stack_entry : public stack_entry
{
    virtual data::instruction_pointer_t instruction_pointer() const override { return ip; }
    virtual std::string_view            symbol_name() const override { return symbol_name_; }

    data::instruction_pointer_t ip;
    std::string_view            symbol_name_;
};

struct perf_data_sample_data : public sample_data
{
    virtual bool has_stack() const override
    {
        return true;
    }

    virtual common::generator<const stack_entry&> reversed_stack() const override
    {
        perf_data_stack_entry current_stack_entry;

        for(const auto instruction_pointer : std::views::reverse(*stack))
        {
            if(instruction_pointer >= std::to_underlying(perf_data::parser::sample_stack_context_marker::max))
            {
                // FIXME: handle context (how to handle that in reversed order??)
                continue;
            }

            const auto [module, load_timestamp] = context->modules_per_process.at(process_id).find(instruction_pointer, timestamp);

            const auto& symbol = (module == nullptr) ?
                                     resolver->make_generic_symbol(instruction_pointer) :
                                     resolver->resolve_symbol(detail::dwarf_resolver::module_info{
                                                                  .image_filename = module->file_name,
                                                                  .image_base     = module->base,
                                                                  .page_offset    = module->page_offset,
                                                                  .process_id     = process_id,
                                                                  .load_timestamp = load_timestamp},
                                                              instruction_pointer);

            current_stack_entry.ip           = instruction_pointer;
            current_stack_entry.symbol_name_ = symbol.name;
            co_yield current_stack_entry;
        }
    }

    const detail::perf_data_file_process_context*   context;
    detail::dwarf_resolver*                         resolver;
    data::process_id_t                              process_id;
    const std::vector<data::instruction_pointer_t>* stack;
    data::timestamp_t                               timestamp;
};

struct perf_data_process_info : process_info
{
    virtual data::process_id_t process_id() const override { return process_id_; }
    virtual data::timestamp_t  start_time() const override { return start_time_; }

    virtual std::string_view image_name() const override { return image_name_; }

    data::process_id_t process_id_;
    data::timestamp_t  start_time_;
    std::string_view   image_name_;
};

} // namespace

perf_data_stack_provider::perf_data_stack_provider(const std::filesystem::path& file_path) :
    file_path_(file_path)
{}

perf_data_stack_provider::~perf_data_stack_provider() = default;

void perf_data_stack_provider::process()
{
    process_context_ = std::make_unique<detail::perf_data_file_process_context>();
    symbol_resolver_ = std::make_unique<detail::dwarf_resolver>();

    perf_data::perf_data_file file(file_path_);
    file.process(process_context_->observer());

    process_context_->finish();
}

common::generator<const process_info&> perf_data_stack_provider::processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    perf_data_process_info current_info;

    for(const auto& [process_id, storage] : process_context.get_samples_per_process())
    {
        const auto* const process = process_context.get_processes().find_at(process_id, storage.first_sample_time);
        assert(process != nullptr);

        current_info.process_id_ = process_id;
        current_info.start_time_ = process->timestamp;
        if(process->payload.name) current_info.image_name_ = *process->payload.name;

        co_yield current_info;
    }
}

common::generator<const sample_data&> perf_data_stack_provider::samples(const process_info& process) const
{
    if(process_context_ == nullptr) co_return;

    assert(symbol_resolver_ != nullptr);

    const auto& process_context = *process_context_;

    const auto& sample_storage = process_context.get_samples_per_process().at(process.process_id());

    perf_data_sample_data current_sample_data;
    current_sample_data.process_id = process.process_id();
    current_sample_data.context    = process_context_.get();
    current_sample_data.resolver   = symbol_resolver_.get();

    for(const auto& sample : sample_storage.samples)
    {
        current_sample_data.stack     = &process_context.stacks.get(sample.stack_index);
        current_sample_data.timestamp = sample.timestamp;
        co_yield current_sample_data;
    }
}
