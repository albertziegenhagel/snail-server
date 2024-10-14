#pragma once

#include <snail/analysis/etl_data_provider.hpp>

namespace snail::analysis {

// The diagsession data provider is basically just an ETL data provider
// that first extracts the ETL file that is contained within a diagsession file.
class diagsession_data_provider : public etl_data_provider
{
public:
    diagsession_data_provider(pdb_symbol_find_options find_options    = {},
                              path_map                module_path_map = {},
                              filter_options          module_filter   = {});

    virtual ~diagsession_data_provider();

    virtual void process(const std::filesystem::path&      file_path,
                         const common::progress_listener*  progress_listener,
                         const common::cancellation_token* cancellation_token) override;

private:
    std::optional<std::filesystem::path> temp_etl_file_path_;

    void try_cleanup() noexcept;
};

} // namespace snail::analysis
