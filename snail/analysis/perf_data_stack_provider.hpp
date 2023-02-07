#pragma once

#include <filesystem>
#include <memory>

#include <snail/analysis/stack_provider.hpp>

namespace snail::analysis {

namespace detail {

class perf_data_file_process_context;
class dwarf_resolver;

} // namespace detail

class perf_data_stack_provider : public stack_provider
{
public:
    explicit perf_data_stack_provider() = default;
    explicit perf_data_stack_provider(const std::filesystem::path& file_path);

    ~perf_data_stack_provider();

    void process();

    virtual common::generator<common::process_id_t> processes() const override;

    virtual common::generator<const sample_data&> samples(common::process_id_t process_id) const override;

private:
    std::filesystem::path file_path_;

    std::unique_ptr<detail::perf_data_file_process_context> process_context_;
    std::unique_ptr<detail::dwarf_resolver>                 symbol_resolver_;
};

} // namespace snail::analysis