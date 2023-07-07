#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace snail::analysis {

struct pdb_symbol_find_options
{
    pdb_symbol_find_options();

    std::vector<std::filesystem::path> search_dirs_;

    std::filesystem::path    symbol_cache_dir_;
    std::vector<std::string> symbol_server_urls_;
};

struct dwarf_symbol_find_options
{
    dwarf_symbol_find_options();

    std::vector<std::filesystem::path> search_dirs_;

    std::filesystem::path    debuginfod_cache_dir_;
    std::vector<std::string> debuginfod_urls_;
};

} // namespace snail::analysis
