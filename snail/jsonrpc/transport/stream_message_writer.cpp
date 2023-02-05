
#include <snail/jsonrpc/transport/stream_message_writer.hpp>

using namespace snail::jsonrpc;

stream_message_writer::stream_message_writer(std::ostream& stream) :
    stream_(&stream)
{}

void stream_message_writer::write(std::string_view content)
{
    (*stream_) << "Content-Length: " << content.size() << "\r\n"
               << "\r\n"
               << content << std::flush;
}
