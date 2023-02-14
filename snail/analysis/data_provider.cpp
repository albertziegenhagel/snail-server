
#include <snail/analysis/data_provider.hpp>

#include <format>

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

std::unique_ptr<data_provider> snail::analysis::make_data_provider(const std::filesystem::path& extension)
{
    if(extension == ".etl")
    {
        return std::make_unique<snail::analysis::etl_data_provider>();
    }
    else if(extension == ".data")
    {
        return std::make_unique<snail::analysis::perf_data_data_provider>();
    }

    throw std::runtime_error(std::format("Unsupported file extension: {}", extension.string()));
}
