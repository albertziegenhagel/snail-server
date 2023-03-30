#pragma once

#include <snail/jsonrpc/protocol.hpp>

namespace snail::jsonrpc {

class v2_protocol : public protocol
{
public:
    [[nodiscard]] virtual request load_request(std::string_view content) override;

    [[nodiscard]] virtual std::string dump_response(const jsonrpc::response& response) override;

    [[nodiscard]] virtual std::string dump_error(const rpc_error& error, const nlohmann::json* id) override;
};

} // namespace snail::jsonrpc
