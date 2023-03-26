
#include <snail/jsonrpc/stream/windows/pipe_streambuf.hpp>

#include <cassert>

#include <Windows.h>

using namespace snail::jsonrpc;

pipe_streambuf::pipe_streambuf() :
    pipe_handle_(INVALID_HANDLE_VALUE)
{
    static_assert(std::is_same_v<HANDLE, void*>);
}

pipe_streambuf::pipe_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode) :
    pipe_streambuf()
{
    open(path, mode);
}

pipe_streambuf::pipe_streambuf(pipe_streambuf&& other) noexcept :
    streambuf_base(std::move(other))
{
    // NOLINTNEXTLINE(bugprone-use-after-move)
    pipe_handle_ = std::exchange(other.pipe_handle_, INVALID_HANDLE_VALUE);
}

pipe_streambuf::~pipe_streambuf()
{
    close();
}

pipe_streambuf& pipe_streambuf::operator=(pipe_streambuf&& other) noexcept
{
    pipe_handle_ = std::exchange(other.pipe_handle_, INVALID_HANDLE_VALUE);
    return *this;
}

void pipe_streambuf::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    streambuf_base::open(path, mode);

    DWORD access = 0;
    if((mode & std::ios_base::in) != 0) access |= GENERIC_READ;
    if((mode & std::ios_base::out) != 0) access |= GENERIC_WRITE;

    pipe_handle_ = CreateFileW(path.c_str(),
                               access,
                               0,
                               nullptr,
                               OPEN_EXISTING,
                               0,
                               nullptr);
}

bool pipe_streambuf::is_open() const
{
    return pipe_handle_ != INVALID_HANDLE_VALUE;
}

void pipe_streambuf::close()
{
    if(pipe_handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }
}

std::streamsize pipe_streambuf::read(char_type* buffer, std::streamsize bytes_to_read)
{
    DWORD      read_bytes;
    const auto result = ReadFile(pipe_handle_, buffer, static_cast<DWORD>(bytes_to_read), &read_bytes, nullptr);
    if(result == FALSE) return 0;
    return static_cast<std::streamsize>(read_bytes);
}

std::streamsize pipe_streambuf::write(const char_type* buffer, std::streamsize bytes_to_write)
{
    DWORD      written_bytes;
    const auto result = WriteFile(pipe_handle_, buffer, static_cast<DWORD>(bytes_to_write), &written_bytes, nullptr);
    if(result == FALSE) return 0;
    return static_cast<std::streamsize>(written_bytes);
}
