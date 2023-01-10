
#include "binarystream.hpp"

using namespace perfreader::utility;

binary_stream_parser::binary_stream_parser(std::istream& stream, std::endian stream_endian) :
    stream_(stream),
    endian_(stream_endian)
{
    // open(file_path, file_endian);
}

// void binary_stream_parser::open(const std::filesystem::path& file_path, std::endian file_endian)
// {
//     stream_.open(file_path, std::ios::binary);
//     if(!stream_.is_open())
//     {
//         // throw std::runtime_error();
//     }
//     endian_ = file_endian;
// }

// void binary_stream_parser::close()
// {
//     stream_.close();
// }

std::size_t binary_stream_parser::read(std::string& data)
{
    std::getline(stream_, data, '\0');
    return data.size() + 1;
}

std::size_t binary_stream_parser::read(std::u16string& data)
{
    data.clear();
    std::uint16_t next_char;
    while(true)
    {
        read(next_char); // TODO: this does include a byteswap. Is this really necessary?
        if(next_char == 0)
        {
            break;
        }
        data.push_back(static_cast<char16_t>(next_char));
    }
    return data.size()*2 + 2;
}

std::size_t binary_stream_parser::read(std::span<std::byte> data)
{
    assert(stream_.good());
    stream_.read(reinterpret_cast<char*>(data.data()), data.size());
    return data.size();
}

bool binary_stream_parser::good() const
{
    return stream_.good();
}

bool binary_stream_parser::has_more_data()
{
    return stream_.good() && stream_.peek() != EOF;
}

void binary_stream_parser::skip(std::streamoff number_of_bytes)
{
    stream_.seekg(number_of_bytes, std::ios_base::cur);
}

std::streampos binary_stream_parser::tell_position()
{
    return stream_.tellg();
}

void binary_stream_parser::set_position(std::streampos position)
{
    stream_.seekg(position, std::ios_base::beg);
}