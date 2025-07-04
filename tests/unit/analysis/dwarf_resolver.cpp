
#include <snail/analysis/detail/dwarf_resolver.hpp>

#include <gtest/gtest.h>

#include <folders.hpp>

#include <snail/analysis/options.hpp>

#include <snail/common/wildcard.hpp>

using namespace snail;
using namespace snail::analysis;
using namespace snail::analysis::detail;
using namespace snail::detail::tests;

namespace {

std::string replace_all(std::string_view str, char search, char replace)
{
    auto result = std::string(str);
    std::ranges::replace(result, search, replace);
    return result;
}

} // namespace

TEST(DwarfResolver, ResolveSymbol)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto exe_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb" / "bin" / "inner";
    ASSERT_TRUE(std::filesystem::exists(exe_path)) << "Missing test file:\n  " << exe_path << "\nDid you forget checking out GIT LFS files?";

    dwarf_resolver resolver;

    const auto exe_path_str = exe_path.string();

    dwarf_resolver::module_info module = {
        .image_filename = std::string_view(exe_path_str),
        .build_id       = {},
        .image_base     = 0x0040'2000,
        .page_offset    = 0x0000'2000,
        .process_id     = 456,
        .load_timestamp = 789};

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x245a + 0xd6 - module.page_offset);

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "compute_inner_product(std::vector<double, std::allocator<double>> const&, std::vector<double, std::allocator<double>> const&)");
        EXPECT_EQ(replace_all(symbol.file_path, '\\', '/'), "/tmp/snail-server/tests/apps/inner/main.cpp");
        EXPECT_EQ(symbol.function_line_number, 26);
        EXPECT_EQ(symbol.instruction_line_number, 39);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x25e0 + 0xbe - module.page_offset);

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "main");
        EXPECT_EQ(replace_all(symbol.file_path, '\\', '/'), "/tmp/snail-server/tests/apps/inner/main.cpp");
        EXPECT_EQ(symbol.function_line_number, 57);
        EXPECT_EQ(symbol.instruction_line_number, 74);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0xFFAA'FFAA + 0);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner!0x00000000ffeb1faa");
        EXPECT_EQ(symbol.file_path, "");
        EXPECT_EQ(symbol.function_line_number, 0);
        EXPECT_EQ(symbol.instruction_line_number, 0);
    }
    {
        const auto symbol = resolver.make_generic_symbol(0xFFAA'FFAA);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "0x00000000ffaaffaa");
        EXPECT_EQ(symbol.file_path, "");
        EXPECT_EQ(symbol.function_line_number, 0);
        EXPECT_EQ(symbol.instruction_line_number, 0);
    }
}

TEST(DwarfResolver, Filter)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto exe_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb" / "bin" / "inner";
    ASSERT_TRUE(std::filesystem::exists(exe_path)) << "Missing test file:\n  " << exe_path << "\nDid you forget checking out GIT LFS files?";

    filter_options filter;
    filter.excluded.emplace_back(common::wildcard_to_regex("*inner"));

    dwarf_resolver resolver({}, {}, filter);

    const auto exe_path_str = exe_path.string();

    dwarf_resolver::module_info module{};
    module.image_filename = exe_path_str;

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x245a + 0xd6 - module.page_offset);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner!0x0000000000002530");
    }
}
