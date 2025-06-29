#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/transport/message_reader.hpp>
#include <snail/jsonrpc/transport/message_writer.hpp>

#include <string_view>

using namespace snail::jsonrpc;

message_connection::message_connection(std::unique_ptr<message_reader> reader,
                                       std::unique_ptr<message_writer> writer) :
    reader_(std::move(reader)),
    writer_(std::move(writer))
{}

message_connection::~message_connection() = default;

void message_connection::serve_next(message_handler& handler)
{
    auto message = reader_->read();

    handler.handle(
        std::move(message),
        [this](std::string_view response)
        {
            writer_->write(response);
        });
}

void message_connection::send(std::string_view message)
{
    writer_->write(message);
}
