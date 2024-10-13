#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/transport/message_reader.hpp>
#include <snail/jsonrpc/transport/message_writer.hpp>

#include <iostream>

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

    std::cout << "<-- " << message << std::endl;

    handler.handle(
        std::move(message),
        [this](std::optional<std::string> response)
        {
            if(!response) return;
            std::cout << "--> " << *response << std::endl;
            writer_->write(*response);
        });
}
