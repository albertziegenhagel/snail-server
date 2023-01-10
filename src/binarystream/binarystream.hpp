
#pragma once

#include <cassert>

#include <istream>
#include <string>
#include <concepts>
#include <bit>

#include <span>

namespace perfreader::utility {

class binary_stream_parser
{
public:
    binary_stream_parser() = default;
    explicit binary_stream_parser(std::istream& stream, std::endian file_endian = std::endian::native);

    // void open(std::istream& stream, std::endian file_endian = std::endian::native);

    // void close();

    template<std::integral T>
    std::size_t read(T& data);

    std::size_t read(std::string& data);
    std::size_t read(std::u16string& data);
    
    std::size_t read(std::span<std::byte> data);

    template<std::integral T>
    T read();

    bool good() const;
    bool has_more_data();

    void skip(std::streamoff number_of_bytes);

    [[nodiscard]] std::streampos tell_position();
    void set_position(std::streampos position);

private:
    std::istream& stream_;
    std::endian endian_;
};

template<std::integral T>
std::size_t binary_stream_parser::read(T& data)
{
    assert(stream_.good());

    stream_.read(reinterpret_cast<char*>(&data), sizeof(T));

    if(std::endian::native != endian_)
    {
        data = std::byteswap(data);
    }

    return sizeof(T);
}

template<std::integral T>
T binary_stream_parser::read()
{
    T data;
    read(data);
    return data;
}

} // namespace perfreader::utility
