
#include <gtest/gtest.h>

#include <snail/analysis/detail/module_map.hpp>

using namespace snail;
using namespace snail::analysis::detail;

TEST(ModuleMap, InsertNonOverlapping)
{
    //  |--|                     module 1 @5
    //          |--|             module 2 @10
    //                  |------| module 3 @3
    const module_info module_1{.base = 10, .size = 20, .file_name = "1"};
    const module_info module_2{.base = 50, .size = 20, .file_name = "2"};
    const module_info module_3{.base = 90, .size = 40, .file_name = "3"};

    {
        module_map map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_1, 5);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(10, 5).first, nullptr);
        EXPECT_EQ(map.find(10, 5).first->file_name, "1");
        EXPECT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(30, 5).first, nullptr);

        map.insert(module_2, 10);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->file_name, "1");

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->file_name, "2");

        map.insert(module_3, 3);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->file_name, "1");

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->file_name, "2");

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->file_name, "3");

        EXPECT_EQ(map.find(5, 20).first, nullptr);
        EXPECT_EQ(map.find(150, 20).first, nullptr);
    }
    {
        module_map map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_3, 3);
        map.insert(module_2, 10);
        map.insert(module_1, 5);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->file_name, "1");

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->file_name, "2");

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->file_name, "3");

        EXPECT_EQ(map.find(5, 20).first, nullptr);
        EXPECT_EQ(map.find(150, 20).first, nullptr);
    }
    {
        module_map map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_1, 5);
        map.insert(module_3, 3);
        map.insert(module_2, 10);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->file_name, "1");

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->file_name, "2");

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->file_name, "3");

        EXPECT_EQ(map.find(5, 20).first, nullptr);
        EXPECT_EQ(map.find(150, 20).first, nullptr);
    }
}

TEST(ModuleMap, InsertOverlapping)
{
    // 0 1 2 3 4 5 6 7 8 9 10
    //   |------------------| module 1 @5
    //     |------|           module 2 @10
    //               |----|   module 3 @3
    //         |--------|     module 4 @20
    const module_info module_1{.base = 10, .size = 100, .file_name = "1"};
    const module_info module_2{.base = 20, .size = 40, .file_name = "2"};
    const module_info module_3{.base = 70, .size = 30, .file_name = "3"};
    const module_info module_4{.base = 40, .size = 50, .file_name = "4"};

    {
        module_map map;

        map.insert(module_1, 5);
        map.insert(module_2, 10);
        map.insert(module_3, 3);
        map.insert(module_4, 20);

        // FIXME: finish tests
    }
}

TEST(ModuleMap, InsertOverlappingExtend)
{
    // 0 1 2 3 4 5 6 7 8 9 10
    //       |--------|       module 1 @5
    //   |------|             module 2 @10
    //               |------| module 3 @3
    //         |--------|     module 4 @20
    const module_info module_1{.base = 30, .size = 50, .file_name = "1"};
    const module_info module_2{.base = 10, .size = 40, .file_name = "2"};
    const module_info module_3{.base = 70, .size = 40, .file_name = "3"};
    const module_info module_4{.base = 40, .size = 50, .file_name = "4"};

    {
        module_map map;

        map.insert(module_1, 5);
        map.insert(module_2, 10);
        map.insert(module_3, 3);
        map.insert(module_4, 20);

        // FIXME: finish tests
    }
}