#pragma once

#include <filesystem>
#include <memory>

#include <snail/analysis/stack_provider.hpp>

namespace snail::analysis {

namespace detail {

class etl_file_process_context;
class pdb_resolver;

} // namespace detail

class etl_stack_provider : public stack_provider
{
public:
    explicit etl_stack_provider() = default;
    explicit etl_stack_provider(const std::filesystem::path& file_path);

    ~etl_stack_provider();

    void process();

    virtual common::generator<common::process_id_t> processes() const override;

    virtual common::generator<const sample_data&> samples(common::process_id_t process_id) const override;

private:
    std::filesystem::path file_path_;

    std::unique_ptr<detail::etl_file_process_context> process_context_;
    std::unique_ptr<detail::pdb_resolver>             symbol_resolver_;
};

} // namespace snail::analysis