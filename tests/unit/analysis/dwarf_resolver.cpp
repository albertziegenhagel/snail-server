
#include <gtest/gtest.h>

#include <folders.hpp>

#include <snail/analysis/detail/dwarf_resolver.hpp>

using namespace snail;
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
        .image_base     = 0x00402000,
        .page_offset    = 0x00002000,
        .process_id     = 456,
        .load_timestamp = 789};

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x245a + 0xd6 - module.page_offset);

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "compute_inner_product(std::vector<double, std::allocator<double>> const&, std::vector<double, std::allocator<double>> const&)");
        EXPECT_EQ(replace_all(symbol.file_path, '\\', '/'), "/tmp/snail-server/tests/apps/inner/main.cpp");
        EXPECT_EQ(symbol.function_line_number, 26);
        EXPECT_EQ(symbol.instruction_line_number, 38);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x25e0 + 0xbe - module.page_offset);

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "main");
        EXPECT_EQ(replace_all(symbol.file_path, '\\', '/'), "/tmp/snail-server/tests/apps/inner/main.cpp");
        EXPECT_EQ(symbol.function_line_number, 57);
        EXPECT_EQ(symbol.instruction_line_number, 73);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0xFFAAFFAA + 0);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner!0x00000000ffeb1faa");
        EXPECT_EQ(symbol.file_path, "");
        EXPECT_EQ(symbol.function_line_number, 0);
        EXPECT_EQ(symbol.instruction_line_number, 0);
    }
    {
        const auto symbol = resolver.make_generic_symbol(0xFFAAFFAA);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "0x00000000ffaaffaa");
        EXPECT_EQ(symbol.file_path, "");
        EXPECT_EQ(symbol.function_line_number, 0);
        EXPECT_EQ(symbol.instruction_line_number, 0);
    }
}
