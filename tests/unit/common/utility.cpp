#include <gtest/gtest.h>

#include <snail/common/bit_flags.hpp>
#include <snail/common/date_time.hpp>
#include <snail/common/filename.hpp>
#include <snail/common/guid.hpp>
#include <snail/common/path.hpp>
#include <snail/common/stream_position.hpp>
#include <snail/common/string_compare.hpp>
#include <snail/common/trim.hpp>
#include <snail/common/wildcard.hpp>

#include <array>

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
    static constexpr const auto allowed_characters = std::array{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'};

    const std::size_t desired_length = 20;

    const auto name = make_random_filename(desired_length);
    EXPECT_EQ(name.size(), desired_length);
    EXPECT_EQ(name.find_first_not_of(allowed_characters.data(), 0, allowed_characters.size()), std::string_view::npos) << "name: " << name;
}

TEST(RandomFilename, Empty)
{
    const auto name = make_random_filename(0);
    EXPECT_EQ(name.size(), 0);
}

TEST(AsciiIEquals, Strings)
{
    EXPECT_TRUE(ascii_iequals("ABC", "ABC"));
    EXPECT_TRUE(ascii_iequals("ABC", "abc"));
    EXPECT_TRUE(ascii_iequals("abc", "ABC"));
    EXPECT_TRUE(ascii_iequals("AbC", "aBc"));

    EXPECT_FALSE(ascii_iequals("ABC", "abcd"));
    EXPECT_FALSE(ascii_iequals("abc", "cba"));
}

enum class test_flags
{
    a = 1,
    b = 2,

    d = 15
};

TEST(BitFlags, Basic)
{
    bit_flags<test_flags, 16> flags;

    const auto& c_flags = flags;

    EXPECT_EQ(flags.count(), 0);
    EXPECT_FALSE(flags.test(test_flags::a));
    EXPECT_FALSE(flags.test(test_flags::b));
    EXPECT_FALSE(flags.test(test_flags::d));
    EXPECT_EQ(flags.data(), std::bitset<16>("0000000000000000"));
    EXPECT_EQ(c_flags.count(), 0);
    EXPECT_FALSE(c_flags.test(test_flags::a));
    EXPECT_FALSE(c_flags.test(test_flags::b));
    EXPECT_FALSE(c_flags.test(test_flags::d));
    EXPECT_EQ(c_flags.data(), std::bitset<16>("0000000000000000"));

    flags.set();
    EXPECT_EQ(flags.count(), 16);
    EXPECT_TRUE(flags.test(test_flags::a));
    EXPECT_TRUE(flags.test(test_flags::b));
    EXPECT_TRUE(flags.test(test_flags::d));
    EXPECT_EQ(flags.data(), std::bitset<16>("1111111111111111"));
    EXPECT_EQ(c_flags.count(), 16);
    EXPECT_TRUE(c_flags.test(test_flags::a));
    EXPECT_TRUE(c_flags.test(test_flags::b));
    EXPECT_TRUE(c_flags.test(test_flags::d));
    EXPECT_EQ(c_flags.data(), std::bitset<16>("1111111111111111"));

    flags.set(test_flags::b, false);
    EXPECT_EQ(flags.count(), 15);
    EXPECT_TRUE(flags.test(test_flags::a));
    EXPECT_FALSE(flags.test(test_flags::b));
    EXPECT_TRUE(flags.test(test_flags::d));
    EXPECT_EQ(flags.data(), std::bitset<16>("1111111111111011"));
    EXPECT_EQ(c_flags.count(), 15);
    EXPECT_TRUE(c_flags.test(test_flags::a));
    EXPECT_FALSE(c_flags.test(test_flags::b));
    EXPECT_TRUE(c_flags.test(test_flags::d));
    EXPECT_EQ(c_flags.data(), std::bitset<16>("1111111111111011"));

    flags.reset();
    EXPECT_EQ(flags.count(), 0);
    EXPECT_FALSE(flags.test(test_flags::a));
    EXPECT_FALSE(flags.test(test_flags::b));
    EXPECT_FALSE(flags.test(test_flags::d));
    EXPECT_EQ(flags.data(), std::bitset<16>("0000000000000000"));
    EXPECT_EQ(c_flags.count(), 0);
    EXPECT_FALSE(c_flags.test(test_flags::a));
    EXPECT_FALSE(c_flags.test(test_flags::b));
    EXPECT_FALSE(c_flags.test(test_flags::d));
    EXPECT_EQ(c_flags.data(), std::bitset<16>("0000000000000000"));

    flags.set(test_flags::a);
    flags.set(test_flags::b, false);
    flags.set(test_flags::d, true);
    EXPECT_EQ(flags.count(), 2);
    EXPECT_TRUE(flags.test(test_flags::a));
    EXPECT_FALSE(flags.test(test_flags::b));
    EXPECT_TRUE(flags.test(test_flags::d));
    EXPECT_EQ(flags.data(), std::bitset<16>("1000000000000010"));
    EXPECT_EQ(c_flags.count(), 2);
    EXPECT_TRUE(c_flags.test(test_flags::a));
    EXPECT_FALSE(c_flags.test(test_flags::b));
    EXPECT_TRUE(c_flags.test(test_flags::d));
    EXPECT_EQ(c_flags.data(), std::bitset<16>("1000000000000010"));
}

TEST(DateTime, FromNtTimestamp)
{
    {
        const auto nt_sys   = from_nt_timestamp(nt_duration(129227643272589782));
        const auto time_str = std::format("{0:%F}T{0:%T%z}", nt_sys);
        EXPECT_EQ(time_str, "2010-07-05T00:45:27.2589782+0000");
    }

    {
        const auto nt_sys   = from_nt_timestamp(nt_duration(133327618442589782));
        const auto time_str = std::format("{0:%F}T{0:%T%z}", nt_sys);
        EXPECT_EQ(time_str, "2023-07-02T08:57:24.2589782+0000");
    }
}

TEST(Guid, Compare)
{
    constexpr auto guid_a = guid{
        0x70ace273, 0x1367, 0x42a0, {0x86, 0x0a, 0x45, 0x4c, 0x34, 0x8c, 0xbd, 0x81}
    };
    constexpr auto guid_b = guid{
        0xabb71d87, 0x4758, 0x40d1, {0xa2, 0x63, 0xc6, 0xea, 0x7d, 0x96, 0x4c, 0x75}
    };

    EXPECT_TRUE(guid_a == guid_a);
    EXPECT_TRUE(guid_b == guid_b);
    EXPECT_FALSE(guid_a == guid_b);
    EXPECT_FALSE(guid_b == guid_a);

    EXPECT_FALSE(guid_a != guid_a);
    EXPECT_FALSE(guid_b != guid_b);
    EXPECT_TRUE(guid_a != guid_b);
    EXPECT_TRUE(guid_b != guid_a);
}

TEST(Guid, ToString)
{
    constexpr auto guid_a = guid{
        0x70ace273, 0x1367, 0x42a0, {0x86, 0x0a, 0x45, 0x4c, 0x34, 0x8c, 0xbd, 0x81}
    };

    EXPECT_EQ(guid_a.to_string(), "70ACE273136742A0860A454C348CBD81");
    EXPECT_EQ(guid_a.to_string(true), "70ACE273-1367-42A0-860A-454C348CBD81");
}

TEST(Guid, Hash)
{
    constexpr auto guid_a = guid{
        0x70ace273, 0x1367, 0x42a0, {0x86, 0x0a, 0x45, 0x4c, 0x34, 0x8c, 0xbd, 0x81}
    };
    constexpr auto guid_b = guid{
        0xabb71d87, 0x4758, 0x40d1, {0xa2, 0x63, 0xc6, 0xea, 0x7d, 0x96, 0x4c, 0x75}
    };

    EXPECT_EQ(std::hash<guid>{}(guid_a), std::hash<guid>{}(guid_a));
    EXPECT_EQ(std::hash<guid>{}(guid_b), std::hash<guid>{}(guid_b));
    EXPECT_NE(std::hash<guid>{}(guid_a), std::hash<guid>{}(guid_b));
}

TEST(Path, FromUtf8)
{
    EXPECT_EQ(path_from_utf8("my-file👻"sv), std::filesystem::path(u8"my-file👻"));
}

TEST(StreamPositionResetter, Reset)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    stream << "DATA DATA DATA DATA DATA";

    EXPECT_EQ(stream.tellg(), 0);
    stream.seekg(5);
    EXPECT_EQ(stream.tellg(), 5);

    {
        auto resetter = stream_position_resetter(stream);
        EXPECT_EQ(stream.tellg(), 5);

        stream.seekg(10);
        EXPECT_EQ(stream.tellg(), 10);
    }

    EXPECT_EQ(stream.tellg(), 5);
}

TEST(Wildcard, ToRegex)
{
    EXPECT_EQ(wildcard_to_regex("*"), R"(^(?:.*)$)");
    EXPECT_EQ(wildcard_to_regex("?"), R"(^(?:.)$)");

    EXPECT_EQ(wildcard_to_regex("*.exe"), R"(^(?:.*\.exe)$)");
    EXPECT_EQ(wildcard_to_regex("C:\\*.dll"), R"(^(?:C:\\.*\.dll)$)");

    EXPECT_EQ(wildcard_to_regex("****.so"), R"(^(?:.*\.so)$)");
    EXPECT_EQ(wildcard_to_regex("filename (1).so"), R"(^(?:filename\ \(1\)\.so)$)");

    EXPECT_EQ(wildcard_to_regex("*ld-linux*.so*"), R"(^(?:.*ld\-linux.*\.so.*)$)");
}
