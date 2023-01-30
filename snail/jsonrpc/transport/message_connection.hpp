#pragma once

#include <memory>

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

    void serve(message_handler& handler);

private:
    std::unique_ptr<message_reader> reader_;
    std::unique_ptr<message_writer> writer_;
};

} // namespace snail::jsonrpc
