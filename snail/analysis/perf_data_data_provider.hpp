#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>

#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

#include <snail/perf_data/build_id.hpp>

namespace snail::analysis {

namespace detail {

class perf_data_file_process_context;
class dwarf_resolver;

} // namespace detail

class perf_data_data_provider : public data_provider
{
public:
    perf_data_data_provider(dwarf_symbol_find_options find_options    = {},
                            path_map                  module_path_map = {},
                            filter_options            module_filter   = {});

    virtual ~perf_data_data_provider();

    virtual void process(const std::filesystem::path&      file_path,
                         const common::progress_listener*  progress_listener,
                         const common::cancellation_token* cancellation_token) override;

    virtual const analysis::session_info& session_info() const override;

    virtual const analysis::system_info& system_info() const override;

    virtual common::generator<unique_process_id> sampling_processes() const override;

    virtual analysis::process_info process_info(unique_process_id process_id) const override;

    virtual common::generator<analysis::thread_info> threads_info(unique_process_id process_id) const override;

    virtual const std::vector<sample_source_info>& sample_sources() const override;

    virtual common::generator<const sample_data&> samples(sample_source_info::id_t source_id,
                                                          unique_process_id        process_id,
                                                          const sample_filter&     filter) const override;

    virtual std::size_t count_samples(sample_source_info::id_t source_id,
                                      unique_process_id        process_id,
                                      const sample_filter&     filter) const override;

private:
    std::unique_ptr<detail::perf_data_file_process_context> process_context_;
    std::unique_ptr<detail::dwarf_resolver>                 symbol_resolver_;

    std::optional<std::unordered_map<std::string, perf_data::build_id>> build_id_map_;
    std::optional<analysis::session_info>                               session_info_;
    std::optional<analysis::system_info>                                system_info_;
    std::vector<sample_source_info>                                     sample_sources_;
    std::vector<std::size_t>                                            sample_source_internal_ids_;

    std::uint64_t session_start_time_;
    std::uint64_t session_end_time_;
};

} // namespace snail::analysis
