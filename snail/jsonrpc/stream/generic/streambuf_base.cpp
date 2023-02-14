
#include <snail/jsonrpc/stream/generic/streambuf_base.hpp>

#include <cassert>

using namespace snail::jsonrpc;

streambuf_base::streambuf_base(streambuf_base&& other) :
    mode_(other.mode_),
    get_area_size_(other.get_area_size_),
    put_area_size_(other.put_area_size_)
{
    this->swap(other);
    buffer_ = std::move(other.buffer_);
}

streambuf_base& streambuf_base::operator=(streambuf_base&& other)
{
    this->swap(other);
    other.setg(nullptr, nullptr, nullptr);
    other.setp(nullptr, nullptr);

    mode_          = other.mode_;
    get_area_size_ = other.get_area_size_;
    put_area_size_ = other.put_area_size_;
    buffer_        = std::move(other.buffer_);

    return *this;
}

void streambuf_base::open(const std::filesystem::path& /*path*/, std::ios_base::openmode mode)
{
    mode_ = mode;
    init_buffer();
}

int streambuf_base::sync()
{
    if(!is_open()) return 0;
    return (overflow(traits_type::eof()) != traits_type::eof()) ? 0 : -1;
}

streambuf_base::int_type streambuf_base::underflow()
{
    if(!is_open()) return traits_type::eof();
    if((mode_ & std::ios_base::in) == 0) return traits_type::eof();

    const auto read_bytes = read(this->eback(), get_area_size_);
    if(read_bytes == 0) return traits_type::eof();

    this->setg(this->eback(), this->eback(), this->eback() + read_bytes);
    return *this->gptr();
}

streambuf_base::int_type streambuf_base::overflow(int_type c)
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

void streambuf_base::init_buffer()
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
