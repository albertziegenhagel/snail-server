#include <gtest/gtest.h>

#include <snail/common/filename.hpp>
#include <snail/common/trim.hpp>

using namespace snail::common;

using namespace std::string_view_literals;

TEST(Trim, Left)
{
    EXPECT_EQ(trim_left("a b\tc\nd   "), "a b\tc\nd   "sv);
    EXPECT_EQ(trim_left("   a b\tc\nd   "), "a b\tc\nd   "sv);
    EXPECT_EQ(trim_left("  \n\t a b\tc\nd   "), "a b\tc\nd   "sv);
}

TEST(Trim, Right)
{
    EXPECT_EQ(trim_right("   a b\tc\nd"), "   a b\tc\nd"sv);
    EXPECT_EQ(trim_right("   a b\tc\nd   "), "   a b\tc\nd"sv);
    EXPECT_EQ(trim_right("   a b\tc\nd \n\t    "), "   a b\tc\nd"sv);
}

TEST(Trim, Both)
{
    EXPECT_EQ(trim("   a b\tc\nd"), "a b\tc\nd"sv);
    EXPECT_EQ(trim("   a b\tc\nd   "), "a b\tc\nd"sv);
    EXPECT_EQ(trim("   a b\tc\nd \n\t    "), "a b\tc\nd"sv);
    EXPECT_EQ(trim("a b\tc\nd   "), "a b\tc\nd"sv);
    EXPECT_EQ(trim("   a b\tc\nd   "), "a b\tc\nd"sv);
    EXPECT_EQ(trim("  \n\t a b\tc\nd   "), "a b\tc\nd"sv);
}
