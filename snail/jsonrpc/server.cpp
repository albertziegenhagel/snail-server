
#include <snail/jsonrpc/server.hpp>

#include <snail/jsonrpc/errors.hpp>
#include <snail/jsonrpc/protocol.hpp>
#include <snail/jsonrpc/transport/message_connection.hpp>

using namespace snail;
using namespace snail::jsonrpc;

server::server(std::unique_ptr<message_connection> connection,
               std::unique_ptr<jsonrpc::protocol>  protocol) :
    connection_(std::move(connection)),
    protocol_(std::move(protocol))
{}

server::~server() = default;

void server::serve_next()
{
    connection_->serve_next(*this);
}

void server::handle(std::string data, respond_callback respond)
{
    jsonrpc::request request;
    try
    {
        request = protocol_->load_request(data);
    }
    catch(rpc_error& e)
    {
        respond(protocol_->dump_error(e, nullptr));
        return;
    }
    catch(std::exception& e)
    {
        respond(protocol_->dump_error(internal_error(e.what()), nullptr));
        return;
    }

    if(request.id)
    {
        const auto& handler = request_handlers_.find(request.method);
        if(handler == request_handlers_.end())
        {
            respond(protocol_->dump_error(unknown_method_error(std::format("Unknown request method: '{}'", request.method.c_str()).c_str()), &(*request.id)));
            return;
        }

        auto respond_wrapper = [this, respond = respond, id = *request.id](nlohmann::json result)
        {
            auto response = jsonrpc::response{
                .result = std::move(result),
                .id     = id};

            auto response_str = protocol_->dump_response(response);

            {
                std::lock_guard<std::mutex> response_guard(response_mutex_);
                respond(response_str);
            }
        };

        auto error_wrapper = [this, respond = respond, id = *request.id](std::string message)
        {
            auto error_str = protocol_->dump_error(internal_error(message.c_str()), &id);

            {
                std::lock_guard<std::mutex> response_guard(response_mutex_);
                respond(error_str);
            }
        };

        handler->second(request.params, respond_wrapper, error_wrapper);
    }
    else
    {
        const auto& handler = notification_handlers_.find(request.method);
        if(handler == notification_handlers_.end())
        {
            respond(protocol_->dump_error(unknown_method_error(std::format("Unknown notification method: '{}'", request.method.c_str()).c_str()), nullptr));
            return;
        }

        auto error_wrapper = [this, respond = std::move(respond)](std::string message)
        {
            auto error_str = protocol_->dump_error(internal_error(message.c_str()), nullptr);

            {
                std::lock_guard<std::mutex> response_guard(response_mutex_);
                respond(std::move(error_str));
            }
        };

        handler->second(request.params, error_wrapper);
    }
}
