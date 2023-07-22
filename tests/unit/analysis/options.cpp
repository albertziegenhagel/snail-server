
#ifdef _WIN32
#    include <Windows.h>
#else
#    include <stdlib.h>
#endif

#include <optional>

#include <gtest/gtest.h>

#include <snail/analysis/options.hpp>

using namespace snail::analysis;

namespace {

void set_environment_variable(const std::string& name, const std::string& value)
{
#ifdef _WIN32
    [[maybe_unused]] const auto success = SetEnvironmentVariableA(name.c_str(), value.c_str());
    assert(success);
#else
    [[maybe_unused]] const auto result = ::setenv(name.c_str(), value.c_str(), 1);
    assert(result == 0);
#endif
}

void unset_environment_variable(const std::string& name)
{
#ifdef _WIN32
    [[maybe_unused]] const auto success = SetEnvironmentVariableA(name.c_str(), nullptr);
    assert(success);
#else
    [[maybe_unused]] const auto result = ::unsetenv(name.c_str());
    assert(result == 0);
#endif
}

} // namespace

TEST(DwarfSymbolOptions, EnvDebuginfodCachePath)
{
    set_environment_variable("DEBUGINFOD_CACHE_PATH", "my/cache/path");
    set_environment_variable("XDG_CACHE_HOME", "my/other/cache/path");

    dwarf_symbol_find_options options;
    EXPECT_EQ(options.debuginfod_cache_dir_.generic_string(), std::filesystem::path("my/cache/path").generic_string());
}

TEST(DwarfSymbolOptions, EnvXdgCacheHome)
{
    unset_environment_variable("DEBUGINFOD_CACHE_PATH");
    set_environment_variable("XDG_CACHE_HOME", "my/other/cache/path");

    dwarf_symbol_find_options options;
    EXPECT_EQ(options.debuginfod_cache_dir_.generic_string(), std::filesystem::path("my/other/cache/path/snail").generic_string());
}

TEST(DwarfSymbolOptions, DebuginfodUrls)
{
    set_environment_variable("DEBUGINFOD_URLS", "https://debuginfod.archlinux.org/ https://debuginfod.elfutils.org/");

    dwarf_symbol_find_options options;
    EXPECT_EQ(options.debuginfod_urls_, (std::vector<std::string>{"https://debuginfod.archlinux.org/", "https://debuginfod.elfutils.org/"}));
}
