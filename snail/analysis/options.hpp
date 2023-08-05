#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace snail::analysis {

enum class filter_mode
{
    all_but_excluded,
    only_included
};

struct filter_options
{
    filter_options();

    filter_mode mode;

    std::vector<std::regex> included;
    std::vector<std::regex> excluded;

    bool check(std::string_view input) const;
};

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

struct options
{
    pdb_symbol_find_options   pdb_find_options;
    dwarf_symbol_find_options dwarf_find_options;

    filter_options filter;
};

struct sample_filter
{
    std::optional<std::chrono::nanoseconds> min_time;
    std::optional<std::chrono::nanoseconds> max_time;

    bool operator==(const sample_filter& other) const = default;
};

} // namespace snail::analysis
