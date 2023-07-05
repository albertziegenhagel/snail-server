
#include <gtest/gtest.h>

#include <snail/analysis/detail/module_map.hpp>

using namespace snail;
using namespace snail::analysis::detail;

struct id_data
{
    int id;

    [[nodiscard]] friend bool operator==(const id_data& lhs, const id_data& rhs)
    {
        return lhs.id == rhs.id;
    }
};

TEST(ModuleMap, InsertNonOverlapping)
{
    //  |--|                     module 1 @5
    //          |--|             module 2 @10
    //                  |------| module 3 @3
    const module_info<id_data> module_1{.base = 10, .size = 20, .payload = {.id = 1}};
    const module_info<id_data> module_2{.base = 50, .size = 20, .payload = {.id = 2}};
    const module_info<id_data> module_3{.base = 90, .size = 40, .payload = {.id = 3}};

    {
        module_map<id_data> map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_1, 5);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(10, 5).first, nullptr);
        EXPECT_EQ(map.find(10, 5).first->payload.id, 1);
        EXPECT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(30, 5).first, nullptr);

        map.insert(module_2, 10);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->payload.id, 1);

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->payload.id, 2);

        map.insert(module_3, 3);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->payload.id, 1);

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->payload.id, 2);

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->payload.id, 3);

        EXPECT_EQ(map.find(5, 20).first, nullptr);
        EXPECT_EQ(map.find(150, 20).first, nullptr);
    }
    {
        module_map<id_data> map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_3, 3);
        map.insert(module_2, 10);
        map.insert(module_1, 5);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->payload.id, 1);

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->payload.id, 2);

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->payload.id, 3);

        EXPECT_EQ(map.find(5, 20).first, nullptr);
        EXPECT_EQ(map.find(150, 20).first, nullptr);
    }
    {
        module_map<id_data> map;

        EXPECT_EQ(map.find(20, 0).first, nullptr);

        map.insert(module_1, 5);
        map.insert(module_3, 3);
        map.insert(module_2, 10);

        EXPECT_EQ(map.find(20, 0).first, nullptr);
        ASSERT_NE(map.find(20, 5).first, nullptr);
        EXPECT_EQ(map.find(20, 5).first->payload.id, 1);

        EXPECT_EQ(map.find(60, 0).first, nullptr);
        EXPECT_EQ(map.find(60, 5).first, nullptr);
        ASSERT_NE(map.find(60, 10).first, nullptr);
        EXPECT_EQ(map.find(60, 10).first->payload.id, 2);

        EXPECT_EQ(map.find(100, 0).first, nullptr);
        EXPECT_EQ(map.find(100, 2).first, nullptr);
        ASSERT_NE(map.find(100, 4).first, nullptr);
        EXPECT_EQ(map.find(100, 4).first->payload.id, 3);

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
    const module_info<id_data> module_1{.base = 10, .size = 100, .payload = {.id = 1}};
    const module_info<id_data> module_2{.base = 20, .size = 40, .payload = {.id = 2}};
    const module_info<id_data> module_3{.base = 70, .size = 30, .payload = {.id = 3}};
    const module_info<id_data> module_4{.base = 40, .size = 50, .payload = {.id = 4}};

    {
        module_map<id_data> map;

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
    const module_info<id_data> module_1{.base = 30, .size = 50, .payload = {.id = 1}};
    const module_info<id_data> module_2{.base = 10, .size = 40, .payload = {.id = 2}};
    const module_info<id_data> module_3{.base = 70, .size = 40, .payload = {.id = 3}};
    const module_info<id_data> module_4{.base = 40, .size = 50, .payload = {.id = 4}};

    {
        module_map<id_data> map;

        map.insert(module_1, 5);
        map.insert(module_2, 10);
        map.insert(module_3, 3);
        map.insert(module_4, 20);

        // FIXME: finish tests
    }
}
