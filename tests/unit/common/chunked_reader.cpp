#include <gtest/gtest.h>

#include <algorithm>

#include <snail/common/chunked_reader.hpp>

using namespace snail::common;

TEST(ChunkedReader, CreateEmpty)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    chunked_reader<8> reader(stream, 0, 0);

    EXPECT_TRUE(reader.done());
    EXPECT_FALSE(reader.chunk_has_more_data());
    EXPECT_FALSE(reader.keep_going());
}

TEST(ChunkedReader, CreateInvalidOffset)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    EXPECT_THROW(chunked_reader<8>(stream, 8, 0), std::runtime_error);
}

TEST(ChunkedReader, ReadInvalidSize)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    chunked_reader<8> reader(stream, 0, 16);

    EXPECT_THROW(reader.read_next_chunk(), std::runtime_error);
}

TEST(ChunkedReader, CreateInvalidState)
{
    std::stringstream stream(std::ios::out | std::ios::binary);
    stream << "DATA"; // will put the stream in an invalid state, since it's not an in-stream.
    EXPECT_THROW(chunked_reader<8>(stream, 0, 0), std::runtime_error);
}

TEST(ChunkedReader, ReadInvalidState)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    stream << "Some test data for the stream";
    chunked_reader<8> reader(stream, 0, 16);

    // puts the stream in an invalid state
    int i;
    stream >> i;

    EXPECT_THROW(reader.read_next_chunk(), std::runtime_error);
}

TEST(ChunkedReader, Read)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);

    stream << "Prefix DATA I AM INTERESTED IN suffix";
    const auto ref_data = stream.str();

    const auto ref_data_1 = std::as_bytes(std::span(ref_data)).subspan(7, 10);
    const auto ref_data_2 = std::as_bytes(std::span(ref_data)).subspan(17, 5);
    const auto ref_data_4 = std::as_bytes(std::span(ref_data)).subspan(22, 8);

    chunked_reader<10> reader(stream, 7, 23);

    EXPECT_TRUE(reader.keep_going());

    const auto data_0 = reader.retrieve_data(10, true);
    EXPECT_TRUE(std::ranges::equal(data_0, ref_data_1));

    const auto data_1 = reader.retrieve_data(10);
    EXPECT_TRUE(std::ranges::equal(data_1, ref_data_1));

    EXPECT_TRUE(reader.keep_going());

    const auto data_2 = reader.retrieve_data(5);
    EXPECT_TRUE(std::ranges::equal(data_2, ref_data_2));

    EXPECT_TRUE(reader.keep_going());

    const auto data_3 = reader.retrieve_data(8);
    EXPECT_TRUE(data_3.empty());

    EXPECT_TRUE(reader.keep_going());

    const auto data_4 = reader.retrieve_data(8);
    EXPECT_TRUE(std::ranges::equal(data_4, ref_data_4));

    EXPECT_FALSE(reader.keep_going());
}

TEST(ChunkedReader, ReadMatching)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);

    stream << "Prefix DATA I AM INTERESTED suffix";
    const auto ref_data = stream.str();

    const auto ref_data_1 = std::as_bytes(std::span(ref_data)).subspan(7, 10);
    const auto ref_data_2 = std::as_bytes(std::span(ref_data)).subspan(17, 10);

    chunked_reader<10> reader(stream, 7, 20);

    EXPECT_TRUE(reader.keep_going());

    const auto data_1 = reader.retrieve_data(10);
    EXPECT_TRUE(std::ranges::equal(data_1, ref_data_1));

    EXPECT_TRUE(reader.keep_going());

    const auto data_2 = reader.retrieve_data(10);
    EXPECT_TRUE(std::ranges::equal(data_2, ref_data_2));

    EXPECT_FALSE(reader.keep_going());
}
