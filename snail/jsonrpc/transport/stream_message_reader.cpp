
#include <snail/jsonrpc/transport/stream_message_reader.hpp>

#include <charconv>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/errors.hpp>

using namespace snail::jsonrpc;

namespace {

enum class content_encoding
{
    utf8
};

struct header
{
    std::size_t      length   = 0;
    content_encoding encoding = content_encoding::utf8;
};

inline constexpr std::string_view content_length_header_name = "Content-Length:";
inline constexpr std::string_view content_type_header_name   = "Content-Type:";

header read_header(std::istream& stream)
{
    header result;

    std::string line;

    while(true)
    {
        std::getline(stream, line);

        if(line.starts_with(content_length_header_name))
        {
            auto offset = content_length_header_name.size();
            while(offset < line.size() && std::isspace(line[offset]) != 0)
                ++offset;
            auto chars_result = std::from_chars(line.data() + offset, line.data() + line.size(), result.length);
            if(chars_result.ec != std::errc{}) throw transport_error("Invalid content length header");
        }
        else if(line.starts_with(content_type_header_name))
        {
            // TODO: parse content type
        }
        else if(line.empty())
        {
            continue; // skip completely empty lines (should they ever happen??)
        }
        else if(line.size() == 1 && line.back() == '\r')
        {
            break; // A line with only a carriage return character marks the end of the header
        }
        else
        {
            throw transport_error(std::format("Invalid header line '{}'", line).c_str());
        }
    }

    return result;
}

} // namespace

stream_message_reader::stream_message_reader(std::istream& stream) :
    stream_(&stream)
{}

std::string stream_message_reader::read()
{
    const auto header = read_header(*stream_);

    if(header.encoding != content_encoding::utf8) throw transport_error("Unsupported content encoding");

    std::string content;
    content.resize(header.length);
    stream_->read(content.data(), header.length);

    return content;
}
