
#include <array>

#include <gtest/gtest.h>

#include <snail/perf_data/build_id.hpp>

using namespace snail::perf_data;

TEST(BuildId, ToString)
{
    const auto id_md5 = build_id{
        .size_   = 16,
        .buffer_ = {
                    std::byte(0xe7), std::byte(0x01), std::byte(0x5f), std::byte(0x7b), std::byte(0x44), std::byte(0x24), std::byte(0x26), std::byte(0x13), std::byte(0xd3), std::byte(0xad),
                    std::byte(0x95), std::byte(0xc2), std::byte(0xe1), std::byte(0x29), std::byte(0x8c), std::byte(0x41), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF)}
    };
    EXPECT_EQ(id_md5.to_string(), "e7015f7b44242613d3ad95c2e1298c41");

    const auto id_sha1 = build_id{
        .size_   = 20,
        .buffer_ = {
                    std::byte(0xef), std::byte(0xdd), std::byte(0x0b), std::byte(0x5e), std::byte(0x69), std::byte(0xb0), std::byte(0x74), std::byte(0x2f), std::byte(0xa5), std::byte(0xe5),
                    std::byte(0xba), std::byte(0xd0), std::byte(0x77), std::byte(0x1d), std::byte(0xf4), std::byte(0xd1), std::byte(0xdf), std::byte(0x24), std::byte(0x59), std::byte(0xd1)}
    };
    EXPECT_EQ(id_sha1.to_string(), "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
}

TEST(BuildId, Compare)
{
    const auto id_md5_a = build_id{
        .size_   = 16,
        .buffer_ = {
                    std::byte(0xe7), std::byte(0x01), std::byte(0x5f), std::byte(0x7b), std::byte(0x44), std::byte(0x24), std::byte(0x26), std::byte(0x13), std::byte(0xd3), std::byte(0xad),
                    std::byte(0x95), std::byte(0xc2), std::byte(0xe1), std::byte(0x29), std::byte(0x8c), std::byte(0x41), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF)}
    };
    const auto id_md5_b = build_id{
        .size_   = 16,
        .buffer_ = {
                    std::byte(0xe7), std::byte(0x01), std::byte(0x5f), std::byte(0x7b), std::byte(0x44), std::byte(0x24), std::byte(0x26), std::byte(0x13), std::byte(0xd3), std::byte(0xad),
                    std::byte(0x95), std::byte(0xc2), std::byte(0xe1), std::byte(0x29), std::byte(0x8c), std::byte(0x41), std::byte(0x00), std::byte(0x00), std::byte(0x00), std::byte(0x00)}
    };
    const auto id_md5_c = build_id{
        .size_   = 16,
        .buffer_ = {
                    std::byte(0x95), std::byte(0xc2), std::byte(0xe1), std::byte(0x7b), std::byte(0x44), std::byte(0x24), std::byte(0x26), std::byte(0x13), std::byte(0xd3), std::byte(0xad),
                    std::byte(0x7b), std::byte(0xe7), std::byte(0x01), std::byte(0x29), std::byte(0x8c), std::byte(0x41), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF)}
    };

    EXPECT_TRUE(id_md5_a == id_md5_a);
    EXPECT_TRUE(id_md5_a == id_md5_b);
    EXPECT_FALSE(id_md5_a == id_md5_c);
    EXPECT_FALSE(id_md5_b == id_md5_c);

    EXPECT_FALSE(id_md5_a != id_md5_a);
    EXPECT_FALSE(id_md5_a != id_md5_b);
    EXPECT_TRUE(id_md5_a != id_md5_c);
    EXPECT_TRUE(id_md5_b != id_md5_c);
}
