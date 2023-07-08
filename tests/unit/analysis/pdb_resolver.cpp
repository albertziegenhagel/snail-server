
#include <gtest/gtest.h>

#include <folders.hpp>

#include <snail/analysis/detail/pdb_resolver.hpp>

using namespace snail;
using namespace snail::analysis::detail;
using namespace snail::detail::tests;

TEST(PdbResolver, ResolveSymbol)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto exe_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb" / "bin" / "inner.exe";
    ASSERT_TRUE(std::filesystem::exists(exe_path)) << "Missing test file:\n  " << exe_path << "\nDid you forget checking out GIT LFS files?";

    pdb_resolver resolver({}, {}, false);

    const auto exe_path_str = exe_path.string();

    pdb_resolver::module_info module = {
        .image_filename = std::string_view(exe_path_str),
        .image_base     = 0x00402000,
        .checksum       = 0,
        .pdb_info       = {},
        .process_id     = 456,
        .load_timestamp = 789};

    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x00001BF0 + 150); // function ends at 0x00001CCB

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "double __cdecl compute_inner_product(class std::vector<double, class std::allocator<double>> const &, class std::vector<double, class std::allocator<double>> const &)");
        EXPECT_EQ(symbol.file_path, "D:\\a\\snail-server\\snail-server\\tests\\apps\\inner\\main.cpp");
        EXPECT_EQ(symbol.function_line_number, 27);
        EXPECT_EQ(symbol.instruction_line_number, 35);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0x00001A80 + 0); // function ends at 0x00001BE2

        EXPECT_FALSE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)");
        EXPECT_EQ(symbol.file_path, "D:\\a\\snail-server\\snail-server\\tests\\apps\\inner\\main.cpp");
        EXPECT_EQ(symbol.function_line_number, 11);
        EXPECT_EQ(symbol.instruction_line_number, 11);
    }
    {
        const auto symbol = resolver.resolve_symbol(module, module.image_base + 0xFFAAFFAA + 0);

        EXPECT_TRUE(symbol.is_generic);
        EXPECT_EQ(symbol.name, "inner.exe!0x00000000ffeb1faa");
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
