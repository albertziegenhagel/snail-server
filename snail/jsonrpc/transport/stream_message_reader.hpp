#pragma once

#include <istream>

#include <snail/jsonrpc/transport/message_reader.hpp>

namespace snail::jsonrpc {

class stream_message_reader : public message_reader
{
public:
    explicit stream_message_reader(std::istream& stream);

    virtual std::string read() override;

private:
    std::istream* stream_;
};

} // namespace snail::jsonrpc
