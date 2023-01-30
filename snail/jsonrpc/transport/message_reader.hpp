#pragma once

#include <string>

namespace snail::jsonrpc {

class message_reader
{
public:
    virtual ~message_reader() = default;

    virtual std::string read() = 0;
};

} // namespace snail::jsonrpc
