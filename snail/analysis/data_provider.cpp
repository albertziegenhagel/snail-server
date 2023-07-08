
#include <snail/analysis/data_provider.hpp>

#include <format>

#include <snail/analysis/diagsession_data_provider.hpp>
#include <snail/analysis/etl_data_provider.hpp>
#include <snail/analysis/perf_data_data_provider.hpp>

// NOTE: The following includes should actually not be required here, but MSVC does not compile without them.
//       Did we miss something?
#include <snail/analysis/detail/dwarf_resolver.hpp>
#include <snail/analysis/detail/etl_file_process_context.hpp>
#include <snail/analysis/detail/pdb_resolver.hpp>
#include <snail/analysis/detail/perf_data_file_process_context.hpp>

using namespace snail;
using namespace snail::analysis;

std::unique_ptr<data_provider> snail::analysis::make_data_provider(const std::filesystem::path& extension,
                                                                   analysis::options            options,
                                                                   path_map                     module_path_map)
{
    if(extension == ".etl")
    {
        return std::make_unique<snail::analysis::etl_data_provider>(std::move(options.pdb_find_options), std::move(module_path_map));
    }
    if(extension == ".diagsession")
    {
        return std::make_unique<snail::analysis::diagsession_data_provider>(std::move(options.pdb_find_options), std::move(module_path_map));
    }
    if(extension == ".data")
    {
        return std::make_unique<snail::analysis::perf_data_data_provider>(std::move(options.dwarf_find_options), std::move(module_path_map));
    }

    throw std::runtime_error(std::format("Unsupported file extension: {}", extension.string()));
}
