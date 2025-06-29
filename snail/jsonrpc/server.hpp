#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/detail/request.hpp>

namespace snail::jsonrpc {

class message_connection;
class protocol;
struct request;
struct response;

using request_id = std::variant<std::int64_t, std::string>;

using request_response_callback = std::function<void(nlohmann::json)>;
using error_callback            = std::function<void(std::string)>;

class server : private message_handler
{
public:
    explicit server(std::unique_ptr<message_connection> connection,
                    std::unique_ptr<jsonrpc::protocol>  protocol);

    ~server();

    void serve_next();

    template<typename RequestType, typename HandlerType>
        requires detail::is_request_v<RequestType> &&
                 std::invocable<HandlerType, const request_id&, RequestType, request_response_callback, error_callback>
    void register_request(HandlerType&& handler);

    template<typename RequestType, typename HandlerType>
        requires detail::is_request_v<RequestType> &&
                 std::invocable<HandlerType, RequestType, error_callback>
    void register_notification(HandlerType&& handler);

    void send_request(const jsonrpc::request& request);

private:
    virtual void handle(std::string data, respond_callback respond) override;

    std::unique_ptr<message_connection> connection_;
    std::unique_ptr<protocol>           protocol_;

    using request_callback      = std::function<void(const nlohmann::json&, const nlohmann::json&, request_response_callback, error_callback)>;
    using notification_callback = std::function<void(const nlohmann::json&, error_callback)>;

    std::unordered_map<std::string, request_callback>      request_handlers_;
    std::unordered_map<std::string, notification_callback> notification_handlers_;

    std::mutex response_mutex_;
};

template<typename RequestType, typename HandlerType>
    requires detail::is_request_v<RequestType> &&
             std::invocable<HandlerType, const request_id&, RequestType, request_response_callback, error_callback>
void server::register_request(HandlerType&& handler)
{
    request_handlers_.emplace(
        std::string(RequestType::name),
        [handler = std::forward<HandlerType>(handler)](const nlohmann::json& id, const nlohmann::json& data, request_response_callback respond, error_callback report_error)
        {
            const auto request_id =
                (id.type() == nlohmann::json::value_t::number_integer ||
                 id.type() == nlohmann::json::value_t::number_unsigned) ?
                    snail::jsonrpc::request_id(id.template get<std::int64_t>()) :
                    snail::jsonrpc::request_id(id.template get<std::string>());
            std::invoke(handler, request_id, detail::unpack_request<RequestType>(data), std::move(respond), std::move(report_error));
        });
}

template<typename RequestType, typename HandlerType>
    requires detail::is_request_v<RequestType> &&
             std::invocable<HandlerType, RequestType, error_callback>
void server::register_notification(HandlerType&& handler)
{
    notification_handlers_.emplace(
        std::string(RequestType::name),
        [handler = std::forward<HandlerType>(handler)](const nlohmann::json& data, error_callback report_error)
        {
            std::invoke(handler, detail::unpack_request<RequestType>(data), std::move(report_error));
        });
}

} // namespace snail::jsonrpc
