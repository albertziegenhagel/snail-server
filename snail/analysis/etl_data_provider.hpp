#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

namespace snail::analysis {

namespace detail {

class etl_file_process_context;
class pdb_resolver;

} // namespace detail

class etl_data_provider : public data_provider
{
public:
    etl_data_provider(pdb_symbol_find_options find_options    = {},
                      path_map                module_path_map = {},
                      filter_options          module_filter   = {});

    virtual ~etl_data_provider();

    virtual void process(const std::filesystem::path& file_path) override;

    virtual const analysis::session_info& session_info() const override;

    virtual const analysis::system_info& system_info() const override;

    virtual common::generator<common::process_id_t> sampling_processes() const override;

    virtual analysis::process_info process_info(common::process_id_t process_id) const override;

    virtual common::generator<analysis::thread_info> threads_info(common::process_id_t process_id) const override;

    virtual common::generator<const sample_data&> samples(common::process_id_t process_id,
                                                          const sample_filter& filter) const override;

private:
    std::unique_ptr<detail::etl_file_process_context> process_context_;
    std::unique_ptr<detail::pdb_resolver>             symbol_resolver_;

    std::optional<analysis::session_info> session_info_;
    std::optional<analysis::system_info>  system_info_;

    std::uint64_t session_start_qpc_ticks_;
    std::uint64_t session_end_qpc_ticks_;
    std::uint64_t qpc_frequency_;
    std::uint32_t pointer_size_;
};

} // namespace snail::analysis
