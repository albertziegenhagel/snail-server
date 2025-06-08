#pragma once

#include <functional>
#include <string>

namespace snail::jsonrpc {

class message_handler
{
public:
    using respond_callback = std::function<void(std::string)>;

    virtual ~message_handler() = default;

    virtual void handle(std::string data, respond_callback respond) = 0;
};

} // namespace snail::jsonrpc
