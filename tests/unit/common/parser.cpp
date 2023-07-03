
#include <snail/common/parser/extract.hpp>

#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace snail::common::parser;

enum class TestEnum : std::int16_t
{
    A = 0x0001,
    B = 0x0100
};

TEST(ParserExtract, Int8)
{
    const std::array<std::uint8_t, 2> data = {
        0x42, 0xAF};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<std::uint8_t>(buffer, 0, std::endian::little), 66);
    EXPECT_EQ(extract<std::int8_t>(buffer, 0, std::endian::little), 66);
    EXPECT_EQ(extract<std::uint8_t>(buffer, 1, std::endian::little), 175);
    EXPECT_EQ(extract<std::int8_t>(buffer, 1, std::endian::little), -81);

    EXPECT_EQ(extract<std::uint8_t>(buffer, 0, std::endian::big), 66);
    EXPECT_EQ(extract<std::int8_t>(buffer, 0, std::endian::big), 66);
    EXPECT_EQ(extract<std::uint8_t>(buffer, 1, std::endian::big), 175);
    EXPECT_EQ(extract<std::int8_t>(buffer, 1, std::endian::big), -81);
}

TEST(ParserExtract, Int16)
{
    const std::array<std::uint8_t, 4> data = {
        0x42, 0xAF, 0xEA, 0x20};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<std::uint16_t>(buffer, 0, std::endian::little), 44866);
    EXPECT_EQ(extract<std::int16_t>(buffer, 0, std::endian::little), -20670);
    EXPECT_EQ(extract<std::uint16_t>(buffer, 2, std::endian::little), 8426);
    EXPECT_EQ(extract<std::int16_t>(buffer, 2, std::endian::little), 8426);

    EXPECT_EQ(extract<std::uint16_t>(buffer, 0, std::endian::big), 17071);
    EXPECT_EQ(extract<std::int16_t>(buffer, 0, std::endian::big), 17071);
    EXPECT_EQ(extract<std::uint16_t>(buffer, 2, std::endian::big), 59936);
    EXPECT_EQ(extract<std::int16_t>(buffer, 2, std::endian::big), -5600);
}

TEST(ParserExtract, Int32)
{
    const std::array<std::uint8_t, 8> data = {
        0xAF, 0x42, 0x00, 0x20, 0x44, 0x7A, 0x77, 0xAE};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<std::uint32_t>(buffer, 0, std::endian::little), 536887983);
    EXPECT_EQ(extract<std::int32_t>(buffer, 0, std::endian::little), 536887983);
    EXPECT_EQ(extract<std::uint32_t>(buffer, 4, std::endian::little), 2927065668);
    EXPECT_EQ(extract<std::int32_t>(buffer, 4, std::endian::little), -1367901628);

    EXPECT_EQ(extract<std::uint32_t>(buffer, 0, std::endian::big), 2940338208);
    EXPECT_EQ(extract<std::int32_t>(buffer, 0, std::endian::big), -1354629088);
    EXPECT_EQ(extract<std::uint32_t>(buffer, 4, std::endian::big), 1148876718);
    EXPECT_EQ(extract<std::int32_t>(buffer, 4, std::endian::big), 1148876718);
}

TEST(ParserExtract, Int64)
{
    const std::array<std::uint8_t, 16> data = {
        0xAF, 0x42, 0x00, 0xAE, 0x44, 0x7A, 0x77, 0x20,
        0xE0, 0x5B, 0xC6, 0x70, 0x04, 0x00, 0x00, 0xE0};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<std::uint64_t>(buffer, 0, std::endian::little), 2339472966837879471ULL);
    EXPECT_EQ(extract<std::int64_t>(buffer, 0, std::endian::little), 2339472966837879471LL);
    EXPECT_EQ(extract<std::uint64_t>(buffer, 8, std::endian::little), 16140901083567774688ULL);
    EXPECT_EQ(extract<std::int64_t>(buffer, 8, std::endian::little), -2305842990141776928LL);

    EXPECT_EQ(extract<std::uint64_t>(buffer, 0, std::endian::big), 12628657053573478176ULL);
    EXPECT_EQ(extract<std::int64_t>(buffer, 0, std::endian::big), -5818087020136073440LL);
    EXPECT_EQ(extract<std::uint64_t>(buffer, 8, std::endian::big), 16166733471782273248ULL);
    EXPECT_EQ(extract<std::int64_t>(buffer, 8, std::endian::big), -2280010601927278368LL);
}

TEST(ParserExtract, Float)
{
    const std::array<std::uint8_t, 8> data = {
        0x10, 0x06, 0x9E, 0xBF, 0xC0, 0xB5, 0xB9, 0x8C};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<float>(buffer, 0, std::endian::little), -1.23456F);
    EXPECT_EQ(extract<float>(buffer, 4, std::endian::little), -2.861315e-31F);

    EXPECT_EQ(extract<float>(buffer, 0, std::endian::big), 2.6549134e-29F);
    EXPECT_EQ(extract<float>(buffer, 4, std::endian::big), -5.6788998F);
}

TEST(ParserExtract, Double)
{
    const std::array<std::uint8_t, 16> data = {
        0x0B, 0x0B, 0xEE, 0x07, 0x3C, 0xDD, 0x5E, 0xC0,
        0xC0, 0x23, 0xC0, 0xCA, 0x45, 0x88, 0xF6, 0x33};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<double>(buffer, 0, std::endian::little), -123.456789);
    EXPECT_EQ(extract<double>(buffer, 8, std::endian::little), 2.2435030420065675e-58);

    EXPECT_EQ(extract<double>(buffer, 0, std::endian::big), 1.8601222332385153e-255);
    EXPECT_EQ(extract<double>(buffer, 8, std::endian::big), -9.8765432099999995);
}

TEST(ParserExtract, Enum)
{
    const std::array<std::uint8_t, 16> data = {
        0x00, 0x01, 0x01, 0x00};
    const auto buffer = std::as_bytes(std::span(data));

    EXPECT_EQ(extract<TestEnum>(buffer, 0, std::endian::little), TestEnum::B);
    EXPECT_EQ(extract<TestEnum>(buffer, 2, std::endian::little), TestEnum::A);

    EXPECT_EQ(extract<TestEnum>(buffer, 0, std::endian::big), TestEnum::A);
    EXPECT_EQ(extract<TestEnum>(buffer, 2, std::endian::big), TestEnum::B);
}

TEST(ParserExtract, U8String)
{
    const std::array<std::uint8_t, 14> data = {
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0xF0, 0x9F, 0x91, 0xBB, 0x00, 0xFF, 0xFF, 0xFF};
    const auto buffer = std::as_bytes(std::span(data));

    std::optional<std::size_t> size;
    EXPECT_EQ(extract_string<char>(buffer, 0, size), "Hello 👻");
    EXPECT_THAT(size, testing::Optional(std::size_t(10)));

    size = 5;
    EXPECT_EQ(extract_string<char>(buffer, 0, size), "Hello");
    EXPECT_THAT(size, testing::Optional(std::size_t(5)));

    size = std::nullopt;
    EXPECT_EQ(extract_string<char>(buffer, 6, size), "👻");
    EXPECT_THAT(size, testing::Optional(std::size_t(4)));
}

TEST(ParserExtract, U16String)
{
    const std::array<std::uint8_t, 21> data = {
        0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x20, 0x00, 0x3D, 0xD8, 0x7B, 0xDC,
        0x00, 0x00, 0xFF, 0xFF, 0xFF};
    const auto buffer = std::as_bytes(std::span(data));

    std::optional<std::size_t> size;
    EXPECT_EQ(extract_string<char16_t>(buffer, 0, size), std::u16string_view(u"Hello 👻"));
    EXPECT_THAT(size, testing::Optional(std::size_t(8)));

    EXPECT_EQ(std::bit_cast<char16_t>(std::uint16_t(0x0048)), u'H');
    size = 5;
    EXPECT_EQ(extract_string<char16_t>(buffer, 0, size), std::u16string_view(u"Hello"));
    EXPECT_THAT(size, testing::Optional(std::size_t(5)));

    size = std::nullopt;
    EXPECT_EQ(extract_string<char16_t>(buffer, 12, size), std::u16string_view(u"👻"));
    EXPECT_THAT(size, testing::Optional(std::size_t(2)));
}
