#pragma once

#include <memory>

#include <snail/jsonrpc/message_handler.hpp>

namespace snail::jsonrpc {

class message_connection;
class protocol;

class server : private message_handler
{
public:
    explicit server(std::unique_ptr<message_connection> connection,
                    std::unique_ptr<jsonrpc::protocol>  protocol);

    ~server();

    void serve();

private:
    virtual std::optional<std::string> handle(std::string_view data) override;

    std::unique_ptr<message_connection> connection_;
    std::unique_ptr<jsonrpc::protocol>  protocol_;

    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::jsonrpc
