
#include "memory_stream.hpp"

using namespace perfreader::utility;

memory_istream_buffer::memory_istream_buffer(std::span<std::byte> buffer) :
    buffer_(buffer)
{
    auto* const pointer = reinterpret_cast<char*>(buffer.data());
    setg(pointer, pointer, pointer + buffer_.size());
}

memory_istream_buffer::pos_type memory_istream_buffer::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
{
    if(dir == std::ios_base::cur)
        gbump(off);
    else if(dir == std::ios_base::end)
        setg(eback(), egptr() + off, egptr());
    else if(dir == std::ios_base::beg)
        setg(eback(), eback() + off, egptr());
    return gptr() - eback();
}

memory_istream_buffer::pos_type memory_istream_buffer::seekpos(pos_type pos, std::ios_base::openmode which)
{
    return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, which);
}
