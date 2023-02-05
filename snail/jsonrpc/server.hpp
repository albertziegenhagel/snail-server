#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/message_handler.hpp>

namespace snail::jsonrpc {

class message_connection;
class protocol;
struct request;
struct response;

class server : private message_handler
{
public:
    explicit server(std::unique_ptr<message_connection> connection,
                    std::unique_ptr<jsonrpc::protocol>  protocol);

    ~server();

    void serve();

    void register_method(std::string name, std::function<std::optional<nlohmann::json>(const nlohmann::json&)> handler);

private:
    virtual std::optional<std::string> handle(std::string_view data) override;

    std::optional<response> handle_request(const request& request);

    std::unique_ptr<message_connection> connection_;
    std::unique_ptr<protocol>           protocol_;

    std::unordered_map<std::string, std::function<std::optional<nlohmann::json>(const nlohmann::json&)>> methods_;
};

} // namespace snail::jsonrpc
