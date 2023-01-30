#pragma once

#include <ostream>

#include <snail/jsonrpc/transport/message_writer.hpp>

namespace snail::jsonrpc {

class stream_message_writer : public message_writer
{
public:
    explicit stream_message_writer(std::ostream& stream);

    virtual void write(std::string_view content) override;

private:
    std::ostream* stream_;
};

} // namespace snail::jsonrpc
