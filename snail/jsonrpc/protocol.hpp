#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace snail::jsonrpc {

struct request
{
    std::string                   method;
    nlohmann::json                params;
    std::optional<nlohmann::json> id;
};

struct response
{
    nlohmann::json                result;
    std::optional<nlohmann::json> id;
};

class rpc_error;

class protocol
{
public:
    virtual ~protocol() = default;

    [[nodiscard]] virtual request load_request(std::string_view content) const = 0;

    [[nodiscard]] virtual std::string dump_request(const jsonrpc::request& request) const = 0;

    [[nodiscard]] virtual std::string dump_response(const jsonrpc::response& response) const = 0;

    [[nodiscard]] virtual std::string dump_error(const rpc_error& error, const nlohmann::json* id) const = 0;
};

} // namespace snail::jsonrpc
