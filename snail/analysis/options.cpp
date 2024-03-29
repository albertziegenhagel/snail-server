#include <snail/analysis/options.hpp>

#include <ranges>

#include <snail/common/path.hpp>
#include <snail/common/system.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

bool matches_any(std::string_view               input,
                 const std::vector<std::regex>& patterns)
{
    for(const auto& pattern : patterns)
    {
        if(std::regex_match(input.begin(), input.end(), pattern)) return true;
    }
    return false;
}

} // namespace

filter_options::filter_options() :
    mode(filter_mode::all_but_excluded)
{}

bool filter_options::check(std::string_view input) const
{
    switch(mode)
    {
    case filter_mode::all_but_excluded:
        return !matches_any(input, excluded);
    case filter_mode::only_included:
        return matches_any(input, included);
    }
    return false;
}

pdb_symbol_find_options::pdb_symbol_find_options() :
    symbol_cache_dir_(common::get_temp_dir() / "SymbolCache")
{
    symbol_server_urls_.push_back("https://msdl.microsoft.com/download/symbols");
}

dwarf_symbol_find_options::dwarf_symbol_find_options()
{
    const auto cache_path_env = common::get_env_var("DEBUGINFOD_CACHE_PATH");
    if(cache_path_env)
    {
        debuginfod_cache_dir_ = *cache_path_env;
    }
    else
    {
        const auto xdg_cache_home_env = common::get_env_var("XDG_CACHE_HOME");
        if(xdg_cache_home_env)
        {
            debuginfod_cache_dir_ = common::path_from_utf8(*xdg_cache_home_env) / "snail";
        }
        else
        {
#ifdef _WIN32
            // On Windows, we don't want to clutter $HOME with a .cache directory by default.
            // Hence we put the symbols into the temp directory by default.
            debuginfod_cache_dir_ = common::get_temp_dir() / "SnailCache";
#else
            // On Linux, default is "$HOME/.cache/snail" and fall-back to
            // "$TEMP/.cache/snail" if we could not determine a valid home directory.
            const auto home_dir   = common::get_home_dir();
            debuginfod_cache_dir_ = (home_dir ? *home_dir : common::get_temp_dir()) / ".cache" / "snail";
#endif
        }
    }

    const auto urls_env = common::get_env_var("DEBUGINFOD_URLS");
    if(urls_env)
    {
        for(const auto url : std::views::split(*urls_env, ' '))
        {
            debuginfod_urls_.push_back(std::string(std::string_view(url)));
        }
    }
}
