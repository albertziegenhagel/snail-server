
#include <snail/jsonrpc/stream/windows/pipe_streambuf.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>

using namespace snail::jsonrpc;

struct pipe_streambuf::impl
{
    HANDLE     pipe_handle_ = INVALID_HANDLE_VALUE;
    OVERLAPPED read_overlapped_;
    OVERLAPPED write_overlapped_;
};

pipe_streambuf::pipe_streambuf() :
    impl_(std::make_unique<impl>())
{
    ZeroMemory(&impl_->read_overlapped_, sizeof(impl_->read_overlapped_));
    ZeroMemory(&impl_->write_overlapped_, sizeof(impl_->write_overlapped_));
}

pipe_streambuf::pipe_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode) :
    pipe_streambuf()
{
    open(path, mode);
}

pipe_streambuf::~pipe_streambuf()
{
    close();
}

void pipe_streambuf::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    streambuf_base::open(path, mode);

    DWORD access = 0;
    if((mode & std::ios_base::in) != 0) access |= GENERIC_READ;
    if((mode & std::ios_base::out) != 0) access |= GENERIC_WRITE;

    impl_->pipe_handle_ = CreateFileW(path.c_str(),
                                      access,
                                      0,
                                      nullptr,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OVERLAPPED,
                                      nullptr);

    ZeroMemory(&impl_->read_overlapped_, sizeof(impl_->read_overlapped_));
    ZeroMemory(&impl_->write_overlapped_, sizeof(impl_->write_overlapped_));

    if((mode & std::ios_base::in) != 0)
    {
        impl_->read_overlapped_.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }
}

bool pipe_streambuf::is_open() const
{
    return impl_->pipe_handle_ != INVALID_HANDLE_VALUE;
}

void pipe_streambuf::close()
{
    if(impl_->pipe_handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(impl_->pipe_handle_);
        impl_->pipe_handle_ = INVALID_HANDLE_VALUE;
    }
    if(impl_->read_overlapped_.hEvent != 0)
    {
        ::CloseHandle(impl_->read_overlapped_.hEvent);
    }
    ZeroMemory(&impl_->read_overlapped_, sizeof(impl_->read_overlapped_));
    ZeroMemory(&impl_->write_overlapped_, sizeof(impl_->write_overlapped_));
}

std::streamsize pipe_streambuf::read(char_type* buffer, std::streamsize bytes_to_read)
{
    DWORD      read_bytes;
    const auto read_result = ReadFile(impl_->pipe_handle_, buffer, static_cast<DWORD>(bytes_to_read), &read_bytes, &impl_->read_overlapped_);
    if(read_result == FALSE && GetLastError() != ERROR_IO_PENDING) return 0;

    const auto overlapped_result = GetOverlappedResult(impl_->pipe_handle_, &impl_->read_overlapped_, &read_bytes, TRUE);
    if(overlapped_result == FALSE) return 0;

    return static_cast<std::streamsize>(read_bytes);
}

std::streamsize pipe_streambuf::write(const char_type* buffer, std::streamsize bytes_to_write)
{
    DWORD      written_bytes;
    const auto write_result = WriteFile(impl_->pipe_handle_, buffer, static_cast<DWORD>(bytes_to_write), &written_bytes, &impl_->write_overlapped_);
    if(write_result == FALSE && GetLastError() != ERROR_IO_PENDING) return 0;

    const auto overlapped_result = GetOverlappedResult(impl_->pipe_handle_, &impl_->write_overlapped_, &written_bytes, TRUE);
    if(overlapped_result == FALSE) return 0;

    return static_cast<std::streamsize>(written_bytes);
}
