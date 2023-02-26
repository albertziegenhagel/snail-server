#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/detail/request.hpp>

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

    template<typename RequestType, typename HandlerType>
        requires detail::is_request_v<RequestType> &&
                 std::invocable<HandlerType, const RequestType&> &&
                 std::convertible_to<std::invoke_result_t<HandlerType, const RequestType&>, nlohmann::json>
    void register_request(HandlerType&& handler);

    template<typename RequestType, typename HandlerType>
        requires detail::is_request_v<RequestType> &&
                 std::invocable<HandlerType, const RequestType&> &&
                 std::is_same_v<std::invoke_result_t<HandlerType, const RequestType&>, void>
    void register_notification(HandlerType&& handler);

private:
    virtual std::optional<std::string> handle(std::string_view data) override;

    std::optional<response> handle_request(const request& request);

    std::unique_ptr<message_connection> connection_;
    std::unique_ptr<protocol>           protocol_;

    void register_method(std::string name, std::function<std::optional<nlohmann::json>(const nlohmann::json&)> handler);

    std::unordered_map<std::string, std::function<std::optional<nlohmann::json>(const nlohmann::json&)>> methods_;
};

template<typename RequestType, typename HandlerType>
    requires detail::is_request_v<RequestType> &&
             std::invocable<HandlerType, const RequestType&> &&
             std::convertible_to<std::invoke_result_t<HandlerType, const RequestType&>, nlohmann::json>
void server::register_request(HandlerType&& handler)
{
    register_method(
        std::string(RequestType::name),
        [handler = std::forward<HandlerType>(handler)](const nlohmann::json& data) -> std::optional<nlohmann::json>
        {
            nlohmann::json response = std::invoke(handler, detail::unpack_request<RequestType>(data));
            return response;
        });
}

template<typename RequestType, typename HandlerType>
    requires detail::is_request_v<RequestType> &&
             std::invocable<HandlerType, const RequestType&> &&
             std::is_same_v<std::invoke_result_t<HandlerType, const RequestType&>, void>
void server::register_notification(HandlerType&& handler)
{
    register_method(
        std::string(RequestType::name),
        [handler = std::forward<HandlerType>(handler)](const nlohmann::json& data) -> std::optional<nlohmann::json>
        {
            std::invoke(handler, detail::unpack_request<RequestType>(data));
            return {};
        });
}

} // namespace snail::jsonrpc
