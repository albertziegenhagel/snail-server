#pragma once

#include <memory>
#include <string_view>

namespace snail::jsonrpc {

class message_reader;
class message_writer;

class message_handler;

class message_connection
{
public:
    explicit message_connection(std::unique_ptr<message_reader> reader,
                                std::unique_ptr<message_writer> writer);

    ~message_connection();

    void serve_next(message_handler& handler);

    void send(std::string_view message);

private:
    std::unique_ptr<message_reader> reader_;
    std::unique_ptr<message_writer> writer_;
};

} // namespace snail::jsonrpc
