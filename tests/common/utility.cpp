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

TEST(RandomFilename, Create)
{
    const char allowed_characters[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    const std::size_t desired_length = 20;

    const auto name = make_random_filename(desired_length);
    EXPECT_EQ(name.size(), desired_length);
    EXPECT_EQ(name.find_first_not_of(allowed_characters), std::string_view::npos) << "name: " << name;
}

TEST(RandomFilename, Empty)
{
    const auto name = make_random_filename(0);
    EXPECT_EQ(name.size(), 0);
}
