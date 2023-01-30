
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

pipe_streambuf::pipe_streambuf(pipe_streambuf&& other) :
    mode_(other.mode_),
    get_area_size_(other.get_area_size_),
    put_area_size_(other.put_area_size_)
{
    this->swap(other);
    buffer_      = std::move(other.buffer_);
    pipe_handle_ = std::exchange(other.pipe_handle_, INVALID_HANDLE_VALUE);
}

pipe_streambuf::~pipe_streambuf()
{
    close();
}

pipe_streambuf& pipe_streambuf::operator=(pipe_streambuf&& other)
{
    this->swap(other);
    other.setg(nullptr, nullptr, nullptr);
    other.setp(nullptr, nullptr);

    mode_          = other.mode_;
    get_area_size_ = other.get_area_size_;
    put_area_size_ = other.put_area_size_;
    buffer_        = std::move(other.buffer_);
    pipe_handle_   = std::exchange(other.pipe_handle_, INVALID_HANDLE_VALUE);

    return *this;
}

void pipe_streambuf::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    mode_ = mode;

    DWORD access = 0;
    if((mode_ & std::ios_base::in) != 0) access |= GENERIC_READ;
    if((mode_ & std::ios_base::out) != 0) access |= GENERIC_WRITE;

    pipe_handle_ = CreateFileW(path.c_str(),
                               access,
                               0,
                               nullptr,
                               OPEN_EXISTING,
                               0,
                               nullptr);

    init_buffer();
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

int pipe_streambuf::sync()
{
    if(!is_open()) return 0;
    return (overflow(traits_type::eof()) != traits_type::eof()) ? 0 : -1;
}

pipe_streambuf::int_type pipe_streambuf::underflow()
{
    if(!is_open()) return traits_type::eof();
    if((mode_ & std::ios_base::in) == 0) return traits_type::eof();

    const auto read_bytes = read(this->eback(), get_area_size_);
    if(read_bytes == 0) return traits_type::eof();

    this->setg(this->eback(), this->eback(), this->eback() + read_bytes);
    return *this->gptr();
}

pipe_streambuf::int_type pipe_streambuf::overflow(int_type c)
{
    if(!is_open()) return traits_type::eof();
    if((mode_ & std::ios_base::out) == 0) return traits_type::eof();

    if(this->pptr() < this->epptr() && c != traits_type::eof())
    {
        return this->sputc(traits_type::to_char_type(c));
    }
    if(this->pbase() == this->epptr())
    {
        if(c == traits_type::eof())
        {
            return traits_type::not_eof(c);
        }
        else
        {
            const auto character = traits_type::to_char_type(c);
            if(write(&character, 1) == 1) return traits_type::not_eof(c);
        }
    }
    else
    {
        const auto pending = this->pptr() - this->pbase();
        const auto written = pending == 0 ? 0 : write(this->pbase(), pending);
        this->pbump(static_cast<int>(-pending));

        if(written == pending)
        {
            if(c == traits_type::eof()) return traits_type::not_eof(c);
            else return this->sputc(traits_type::to_char_type(c));
        }
    }
    return traits_type::eof();
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

void pipe_streambuf::init_buffer()
{
    static constexpr std::size_t buffer_size = BUFSIZ;

    buffer_ = std::make_unique<char[]>(buffer_size);

    const auto half_buffer_size = std::ldiv(static_cast<long int>(buffer_size), static_cast<long int>(2));
    get_area_size_              = half_buffer_size.quot + half_buffer_size.rem;
    put_area_size_              = half_buffer_size.quot;

    auto* const get_buffer = buffer_.get();
    auto* const put_buffer = buffer_.get() + get_area_size_;

    if((mode_ & std::ios_base::in) != 0) this->setg(get_buffer, get_buffer, get_buffer);
    if((mode_ & std::ios_base::out) != 0) this->setp(put_buffer, put_buffer + put_area_size_);
}
