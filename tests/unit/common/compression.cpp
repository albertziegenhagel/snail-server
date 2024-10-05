
#include <array>
#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <snail/common/ms_xca_decompression.hpp>

#include <random>
#include <snail/common/detail/dump.hpp>

using namespace snail;

// using fpRtlDecompressBufferEx          = NTSTATUS(__stdcall*)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG, PVOID);
// using fpRtlCompressBuffer              = NTSTATUS(__stdcall*)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
// using fpRtlGetCompressionWorkSpaceSize = NTSTATUS(__stdcall*)(USHORT, PULONG, PULONG);

// void build_test_data()
// {
//     HMODULE ntdll = GetModuleHandle("ntdll.dll");

//     const auto RtlDecompressBufferEx          = (fpRtlDecompressBufferEx)GetProcAddress(ntdll, "RtlDecompressBufferEx");
//     const auto RtlCompressBuffer              = (fpRtlCompressBuffer)GetProcAddress(ntdll, "RtlCompressBuffer");
//     const auto RtlGetCompressionWorkSpaceSize = (fpRtlGetCompressionWorkSpaceSize)GetProcAddress(ntdll, "RtlGetCompressionWorkSpaceSize");

//     ULONG      buffer_workspace_size;
//     ULONG      fragment_workspace_size;
//     const auto wr = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_XPRESS, &buffer_workspace_size, &fragment_workspace_size);

//     const auto workspace = std::make_unique<std::byte[]>(buffer_workspace_size);

//     std::string input_str = "abcdef"
//                             "gg"
//                             "hhh"
//                             "iiii"
//                             "jjjjj"
//                             "kkkkkkkkkk"
//                             "jjjjjjjjjjjjjjjjjjjj"
//                             "llllllllllllllllllllllllllllllllllllllll"
//                             "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
//                             "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
//                             "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
//     for(std::size_t i = 0; i < (0xFFFF + 5); ++i)
//     {
//         input_str.push_back('q');
//     }

//     auto ori_decompressed_buffer = std::make_unique<std::byte[]>(input_str.size());
//     std::memcpy(ori_decompressed_buffer.get(), input_str.data(), input_str.size());
//     auto compressed_buffer = std::make_unique<std::byte[]>(1024);

//     ULONG      compressed_size;
//     const auto cr = RtlCompressBuffer(
//         COMPRESSION_FORMAT_XPRESS,
//         reinterpret_cast<unsigned char*>(ori_decompressed_buffer.get()),
//         input_str.size(),
//         reinterpret_cast<unsigned char*>(compressed_buffer.get()),
//         1024,
//         4096,
//         &compressed_size,
//         workspace.get());

//     common::detail::dump_buffer(
//         std::span(compressed_buffer.get(), compressed_size), 0, compressed_size);

//     auto decompressed_buffer = std::make_unique<std::byte[]>(input_str.size());

//     // ULONG decompressed_size;
//     // const auto dr = RtlDecompressBufferEx(
//     //     COMPRESSION_FORMAT_XPRESS,
//     //     reinterpret_cast<unsigned char*>(decompressed_buffer.get()),
//     //     input_str.size(),
//     //     reinterpret_cast<unsigned char*>(compressed_buffer.get()),
//     //     compressed_size,
//     //     &decompressed_size,
//     //     workspace.get());
// }

TEST(MsXcaCompression, DecompressXPress)
{
    const std::array<std::uint8_t, 59> compressed_buffer = {
        0xaf, 0xea, 0x0a, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x67, 0x68, 0x68, 0x68, 0x69,
        0x00, 0x00, 0x6a, 0x01, 0x00, 0x6b, 0x06, 0x00, 0x72, 0x00, 0x27, 0x00, 0xf5, 0x6c, 0x07, 0x00,
        0x0e, 0x6d, 0x07, 0x00, 0xff, 0x36, 0x6f, 0x07, 0x00, 0x86, 0x70, 0x07, 0x00, 0xff, 0xff, 0x3c,
        0x01, 0x71, 0x07, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

    const auto expected_decompressed = []()
    {
        std::string result = "abcdef"
                             "gg"
                             "hhh"
                             "iiii"
                             "jjjjj"
                             "kkkkkkkkkk"
                             "jjjjjjjjjjjjjjjjjjjj"
                             "llllllllllllllllllllllllllllllllllllllll"
                             "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
                             "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
                             "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
        for(std::size_t i = 0; i < (0xFFFF + 5); ++i)
        {
            result.push_back('q');
        }
        return result;
    }();

    std::string decompressed_data;
    decompressed_data.resize(expected_decompressed.size());

    const auto decompressed_size = common::ms_xca_decompress(
        std::as_bytes(std::span(compressed_buffer)),
        std::as_writable_bytes(std::span(decompressed_data.data(), decompressed_data.size())),
        common::ms_xca_compression_format::xpress);

    EXPECT_EQ(decompressed_size, expected_decompressed.size());
    EXPECT_EQ(decompressed_data, expected_decompressed);
}

TEST(MsXcaCompression, DecompressXPressInsufficient)
{
    const std::array<std::uint8_t, 59> compressed_buffer = {
        0xaf, 0xea, 0x0a, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x67, 0x68, 0x68, 0x68, 0x69,
        0x00, 0x00, 0x6a, 0x01, 0x00, 0x6b, 0x06, 0x00, 0x72, 0x00, 0x27, 0x00, 0xf5, 0x6c, 0x07, 0x00,
        0x0e, 0x6d, 0x07, 0x00, 0xff, 0x36, 0x6f, 0x07, 0x00, 0x86, 0x70, 0x07, 0x00, 0xff, 0xff, 0x3c,
        0x01, 0x71, 0x07, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

    EXPECT_THAT(
        ([&]()
         {
            std::array<std::uint8_t, 0> decompressed_buffer;
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::xpress); }),
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Insufficient output buffer size")));

    EXPECT_THAT(
        ([&]()
         {
            std::array<std::uint8_t, 512> decompressed_buffer;
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::xpress); }),
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Insufficient output buffer size")));
}

TEST(MsXcaCompression, DecompressXPressInvalid)
{
    std::array<std::uint8_t, 1024> decompressed_buffer;

    const std::array<std::uint8_t, 16> compressed_random_buffer = {
        0x3f, 0xfb, 0x29, 0x26, 0xc8, 0x55, 0xce, 0x49, 0xa0, 0x27, 0x9a, 0x4f, 0xe2, 0x1f, 0xc2, 0xd1};

    EXPECT_THAT(
        [&]()
        {
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_random_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::xpress);
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid compressed data")));

    const std::array<std::uint8_t, 59> compressed_rigged_buffer = {
        0xaf, 0xea, 0x0a, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x67, 0x68, 0x68, 0x68, 0x69,
        0x00, 0x00, 0x6a, 0x01, 0x00, 0x6b, 0x06, 0x00, 0x72, 0x00, 0x27, 0x00, 0xf5, 0x6c, 0x07, 0x00,
        0x0e, 0x6d, 0x07, 0x00, 0xff, 0x36, 0x6f, 0x07, 0x00, 0x86, 0x70, 0x07, 0x00, 0xff, 0xff, 0x3c,
        0x01, 0x71, 0x07, 0x00, 0xff, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};

    EXPECT_THAT(
        [&]()
        {
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_rigged_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::xpress);
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid compressed data")));
}

TEST(MsXcaCompression, DecompressLznt1)
{
    const std::array<std::uint8_t, 1> compressed_buffer = {0x00};

    std::array<std::uint8_t, 1> decompressed_buffer;

    EXPECT_THAT(
        [&]()
        {
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::lznt1);
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("not yet implemented")));
}

TEST(MsXcaCompression, DecompressXPressHuff)
{
    const std::array<std::uint8_t, 1> compressed_buffer = {0x00};

    std::array<std::uint8_t, 1> decompressed_buffer;

    EXPECT_THAT(
        [&]()
        {
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format::xpress_huff);
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("not yet implemented")));
}

TEST(MsXcaCompression, DecompressInvalid)
{
    const std::array<std::uint8_t, 1> compressed_buffer = {0x00};

    std::array<std::uint8_t, 1> decompressed_buffer;

    EXPECT_THAT(
        [&]()
        {
            common::ms_xca_decompress(
                std::as_bytes(std::span(compressed_buffer)),
                std::as_writable_bytes(std::span(decompressed_buffer)),
                common::ms_xca_compression_format(1234));
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid compression format")));
}

TEST(MsXcaCompression, DecompressNone)
{
    const std::string_view input_data = "Lorem ipsum dolor sit amet";

    std::string decompressed_data;
    decompressed_data.resize(input_data.size());

    const auto decompressed_size = common::ms_xca_decompress(
        std::as_bytes(std::span(input_data)),
        std::as_writable_bytes(std::span(decompressed_data.data(), decompressed_data.size())),
        common::ms_xca_compression_format::none);

    EXPECT_EQ(decompressed_size, input_data.size());
    EXPECT_EQ(decompressed_data, input_data);
}
