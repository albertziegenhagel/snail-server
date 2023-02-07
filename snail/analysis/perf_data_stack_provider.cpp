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
    virtual common::instruction_pointer_t instruction_pointer() const override { return ip; }
    virtual std::string_view              symbol_name() const override { return symbol_name_; }
    virtual std::string_view              module_name() const override { return module_name_; }

    common::instruction_pointer_t ip;
    std::string_view              symbol_name_;
    std::string_view              module_name_;
};

struct perf_data_sample_data : public sample_data
{
    virtual bool has_stack() const override
    {
        return true;
    }

    virtual common::generator<const stack_entry&> reversed_stack() const override
    {
        static const std::string unkown_module_name = "[unknown]";

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
            current_stack_entry.module_name_ = module == nullptr ? unkown_module_name : module->file_name;
            co_yield current_stack_entry;
        }
    }

    const detail::perf_data_file_process_context*     context;
    detail::dwarf_resolver*                           resolver;
    common::process_id_t                              process_id;
    const std::vector<common::instruction_pointer_t>* stack;
    common::timestamp_t                               timestamp;
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

common::generator<common::process_id_t> perf_data_stack_provider::processes() const
{
    if(process_context_ == nullptr) co_return;

    const auto& process_context = *process_context_;

    for(const auto& [process_id, storage] : process_context.get_samples_per_process())
    {
        co_yield common::process_id_t(process_id);
    }
}

common::generator<const sample_data&> perf_data_stack_provider::samples(common::process_id_t process_id) const
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
        current_sample_data.stack     = &process_context.stacks.get(sample.stack_index);
        current_sample_data.timestamp = sample.timestamp;
        co_yield current_sample_data;
    }
}
