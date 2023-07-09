#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/transport/message_reader.hpp>
#include <snail/jsonrpc/transport/message_writer.hpp>

using namespace snail::jsonrpc;

message_connection::message_connection(std::unique_ptr<message_reader> reader,
                                       std::unique_ptr<message_writer> writer) :
    reader_(std::move(reader)),
    writer_(std::move(writer))
{}

message_connection::~message_connection() = default;

[[noreturn]] void message_connection::serve_forever(message_handler& handler)
{
    while(true)
    {
        serve_next(handler);
    }
}

void message_connection::serve_next(message_handler& handler)
{
    auto message = reader_->read();

    const auto result = handler.handle(message);
    if(result)
    {
        writer_->write(*result);
    }
}
