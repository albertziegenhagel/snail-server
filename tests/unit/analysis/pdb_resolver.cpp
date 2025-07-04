
#include <snail/analysis/detail/pdb_resolver.hpp>

#include <gtest/gtest.h>

#include <folders.hpp>

#include <snail/analysis/options.hpp>

#include <snail/common/wildcard.hpp>

using namespace snail;
using namespace snail::analysis;
using namespace snail::analysis::detail;
using namespace snail::detail::tests;

TEST(PdbResolver, ResolveSymbol)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto exe_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb" / "bin" / "inner.exe";
    ASSERT_TRUE(std::filesystem::exists(exe_path)) << "Missing test file:\n  " << exe_path << "\nDid you forget checking out GIT LFS files?";

    pdb_resolver resolver({}, {}, {}, false);

    const auto exe_path_str = exe_path.string();

    pdb_resolver::module_info module = {
        .image_filename = std::string_view(exe_path_str),
        .image_base     = 0x0040'2000,
        .checksum       = 0,
        .pdb_info       = {},
        .process_id     = 456,
        .load_timestamp = 789};

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x0000'1BF0 + 150); // function ends at 0x00001CCB

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "double __cdecl compute_inner_product(class std::vector<double, class std::allocator<double>> const &, class std::vector<double, class std::allocator<double>> const &)");
        EXPECT_EQ(symbol.file_path, "D:\\a\\snail-server\\snail-server\\tests\\apps\\inner\\main.cpp");
        EXPECT_EQ(symbol.function_line_number, 28);
        EXPECT_EQ(symbol.instruction_line_number, 36);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x0000'1A80 + 0); // function ends at 0x00001BE2

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)");
        EXPECT_EQ(symbol.file_path, "D:\\a\\snail-server\\snail-server\\tests\\apps\\inner\\main.cpp");
        EXPECT_EQ(symbol.function_line_number, 12);
        EXPECT_EQ(symbol.instruction_line_number, 12);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0xFFAA'FFAA + 0);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner.exe!0x00000000ffeb1faa");
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

TEST(PdbResolver, Filter)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto exe_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb" / "bin" / "inner.exe";
    ASSERT_TRUE(std::filesystem::exists(exe_path)) << "Missing test file:\n  " << exe_path << "\nDid you forget checking out GIT LFS files?";

    filter_options filter;
    filter.excluded.emplace_back(common::wildcard_to_regex("*inner.exe"));

    pdb_resolver resolver({}, {}, filter, false);

    const auto exe_path_str = exe_path.string();

    pdb_resolver::module_info module{};
    module.image_filename = exe_path_str;

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x0000'1BF0 + 150);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner.exe!0x0000000000001c86");
    }
}
