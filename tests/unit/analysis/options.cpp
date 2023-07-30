
#ifdef _WIN32
#    include <Windows.h>
#else
#    include <stdlib.h>
#endif

#include <optional>

#include <gtest/gtest.h>

#include <snail/analysis/options.hpp>

#include <snail/common/wildcard.hpp>

using namespace snail::common;
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

TEST(FilterOptions, AllButExcludedWildcards)
{
    filter_options filter;
    filter.mode = filter_mode::all_but_excluded;

    filter.excluded.emplace_back(wildcard_to_regex("*.xyz"));
    filter.excluded.emplace_back(wildcard_to_regex("C:\\Windows\\*"));
    filter.excluded.emplace_back(wildcard_to_regex("C:\\windows\\*"));
    filter.excluded.emplace_back(wildcard_to_regex("C:\\WINDOWS\\*"));
    filter.excluded.emplace_back(wildcard_to_regex("*libc.so*"));
    filter.excluded.emplace_back(wildcard_to_regex("*ld-linux*.so*"));

    EXPECT_TRUE(filter.check(R"(C:\my-file-somewhere.exe)"));
    EXPECT_FALSE(filter.check(R"(C:\path\to\shared.xyz)"));
    EXPECT_FALSE(filter.check(R"(C:\Windows\System32\ntdll.dll)"));
    EXPECT_FALSE(filter.check(R"(C:\WINDOWS\System32\kernel32.dll)"));
    EXPECT_TRUE(filter.check(R"(abc.txt)"));
    EXPECT_FALSE(filter.check(R"(def.xyz)"));
    EXPECT_TRUE(filter.check(R"(a.xyz.file)"));
    EXPECT_FALSE(filter.check(R"(/lib/libc.so)"));
    EXPECT_FALSE(filter.check(R"(/lib/libc.so.6)"));
    EXPECT_FALSE(filter.check(R"(/usr/lib64/ld-linux-x86-64.so.2)"));
}

TEST(FilterOptions, OnlyIncludedWildcards)
{
    filter_options filter;
    filter.mode = filter_mode::only_included;

    filter.included.emplace_back(wildcard_to_regex("/my/important/path/*"));
    filter.included.emplace_back(wildcard_to_regex("*my-important-file.dll"));
    filter.included.emplace_back(wildcard_to_regex("C:\\Windows\\**"));
    filter.included.emplace_back(wildcard_to_regex("*libc.so*"));

    EXPECT_FALSE(filter.check(R"(C:\some-file.exe)"));
    EXPECT_FALSE(filter.check(R"(C:\my\important\path\some-file.exe)"));
    EXPECT_TRUE(filter.check(R"(/my/important/path/some-file.exe)"));
    EXPECT_TRUE(filter.check(R"(/some/path/my-important-file.dll)"));
    EXPECT_FALSE(filter.check(R"(/some/path/my-important-file.exe)"));
    EXPECT_TRUE(filter.check(R"(C:\Windows\System32\ntdll.dll)"));
    EXPECT_TRUE(filter.check(R"(/lib/libc.so)"));
    EXPECT_TRUE(filter.check(R"(/lib/libc.so.6)"));
    EXPECT_FALSE(filter.check(R"(/lib/libstdc++.so)"));
}
