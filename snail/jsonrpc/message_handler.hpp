#pragma once

#include <optional>
#include <string>

namespace snail::jsonrpc {

class message_handler
{
public:
    virtual ~message_handler() = default;

    virtual std::optional<std::string> handle(std::string_view data) = 0;
};

} // namespace snail::jsonrpc
