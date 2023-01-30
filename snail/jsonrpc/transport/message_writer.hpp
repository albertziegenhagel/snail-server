#pragma once

#include <string>

namespace snail::jsonrpc {

class message_writer
{
public:
    virtual ~message_writer() = default;

    virtual void write(std::string_view content) = 0;
};

} // namespace snail::jsonrpc
