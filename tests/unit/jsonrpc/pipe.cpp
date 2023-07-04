
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <gtest/gtest.h>

#include <snail/jsonrpc/stream/windows/pipe_iostream.hpp>

#include <fstream>

using namespace snail::jsonrpc;

struct named_pipe
{
    named_pipe(const std::filesystem::path& path)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
        handle = CreateNamedPipeW(path.c_str(),
                                  PIPE_ACCESS_DUPLEX,
                                  PIPE_TYPE_BYTE | PIPE_NOWAIT,
                                  1,
                                  64,
                                  64,
                                  0,
                                  nullptr);

        if(handle == INVALID_HANDLE_VALUE) throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
    }

    ~named_pipe()
    {
        if(handle == INVALID_HANDLE_VALUE) return;
        CloseHandle(handle);
    }

    HANDLE handle;
};

TEST(PipeIoStream, DefaultConstructOpen)
{
    const auto* const pipe_path = R"(\\.\pipe\snail-test-pipe-1)";
    const auto        pipe      = named_pipe(pipe_path);

    pipe_iostream stream;

    EXPECT_FALSE(stream.is_open());

    stream.open(pipe_path);

    EXPECT_TRUE(stream.is_open());
}

TEST(PipeIoStream, ConstructOpen)
{
    const auto* const pipe_path = R"(\\.\pipe\snail-test-pipe-2)";
    const auto        pipe      = named_pipe(pipe_path);

    const pipe_iostream stream(pipe_path);
    EXPECT_TRUE(stream.is_open());
}

TEST(PipeIoStream, ConstructOpenInvalid)
{
    const auto* const   pipe_path = R"(\\.\pipe\snail-non-existing-pipe)";
    const pipe_iostream stream(pipe_path);
    EXPECT_FALSE(stream.is_open());
}

TEST(PipeIoStream, ReadWrite)
{
    using namespace std::literals;

    const auto* const pipe_path = R"(\\.\pipe\snail-test-pipe-3)";
    const auto        pipe      = named_pipe(pipe_path);

    // pipe_iostream stream(pipe_path);

    std::fstream stream(pipe_path);
    EXPECT_TRUE(stream.is_open());

    // const auto x = std::filesystem::is_socket(stream);

    const auto test_data_in = "in-data\n"sv;

    DWORD      temp_bytes_count;
    const auto write_result = WriteFile(pipe.handle, test_data_in.data(), static_cast<DWORD>(test_data_in.size()), &temp_bytes_count, nullptr);
    EXPECT_TRUE(write_result);
    EXPECT_EQ(temp_bytes_count, test_data_in.size());

    std::string buffer;
    buffer.resize(test_data_in.size());
    stream.read(buffer.data(), static_cast<std::streamsize>(test_data_in.size()));
    EXPECT_EQ(buffer, test_data_in);

    const auto test_data_out = "out-data\n"sv;

    stream.write(test_data_out.data(), static_cast<std::streamsize>(test_data_out.size()));
    stream.flush();

    buffer.clear();
    buffer.resize(test_data_out.size() + 1);
    buffer.back()          = '\0';
    const auto read_result = ReadFile(pipe.handle, buffer.data(), static_cast<DWORD>(test_data_out.size()), &temp_bytes_count, nullptr);
    EXPECT_TRUE(read_result);
    EXPECT_EQ(temp_bytes_count, test_data_out.size());
}
