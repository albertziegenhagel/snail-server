
#include <gtest/gtest.h>

#include <snail/analysis/detail/stack_cache.hpp>

using namespace snail;
using namespace snail::analysis::detail;

TEST(StackCache, Insert)
{
    stack_cache cache;

    const auto stack_1 = std::vector<std::size_t>{
        123,
        456,
        789};
    const auto stack_1_2 = std::vector<std::size_t>{
        123,
        456,
        789};
    const auto stack_2 = std::vector<std::size_t>{
        123,
        456};
    const auto stack_3 = std::vector<std::size_t>{
        789,
        456,
        123};

    const auto index_1 = cache.insert(stack_1);
    EXPECT_EQ(cache.get(index_1), stack_1);

    const auto index_1_2 = cache.insert(stack_1_2);
    EXPECT_EQ(index_1_2, index_1);
    EXPECT_EQ(cache.get(index_1), stack_1_2);
    EXPECT_EQ(cache.get(index_1_2), stack_1_2);

    const auto index_2 = cache.insert(stack_2);
    EXPECT_NE(index_2, index_1);
    EXPECT_EQ(cache.get(index_2), stack_2);
    EXPECT_NE(cache.get(index_1), stack_2);

    const auto index_3 = cache.insert(stack_3);
    EXPECT_NE(index_3, index_1);
    EXPECT_NE(index_3, index_2);
    EXPECT_EQ(cache.get(index_3), stack_3);
    EXPECT_NE(cache.get(index_2), stack_3);
    EXPECT_NE(cache.get(index_1), stack_3);

    const auto index_4 = cache.insert(std::span(stack_1).subspan(0, 2));
    EXPECT_EQ(index_4, index_2);
}
