
#include <snail/jsonrpc/server.hpp>

#include <snail/jsonrpc/errors.hpp>
#include <snail/jsonrpc/protocol.hpp>
#include <snail/jsonrpc/transport/message_connection.hpp>

using namespace snail::jsonrpc;

struct server::impl
{
    std::optional<jsonrpc::response> handle_request(const jsonrpc::request& request)
    {
        const auto& method = methods_.find(request.method);
        if(method == methods_.end()) throw unknown_method_error(request.method.c_str());

        auto result = method->second(request.params);

        if(!result) return std::nullopt;

        return response{
            .result = std::move(*result),
            .id     = request.id};
    }

    std::unordered_map<std::string, std::function<std::optional<nlohmann::json>(const nlohmann::json&)>> methods_;
};

server::server(std::unique_ptr<message_connection> connection,
               std::unique_ptr<jsonrpc::protocol>  protocol) :
    connection_(std::move(connection)),
    protocol_(std::move(protocol)),
    impl_(std::make_unique<impl>())
{}

server::~server() = default;

void server::serve()
{
    connection_->serve(*this);
}

std::optional<std::string> server::handle(std::string_view data)
{
    const nlohmann::json* error_id = nullptr;
    try
    {
        auto request = protocol_->load_request(data);

        if(request.id != std::nullopt)
        {
            error_id = &request.id.value();
        }

        auto response = impl_->handle_request(request);

        if(!response) return std::nullopt;

        return protocol_->dump_response(*response);
    }
    catch(rpc_error& e)
    {
        return protocol_->dump_error(e, error_id);
    }
    catch(std::exception& e)
    {
        return protocol_->dump_error(internal_error(e.what()), error_id);
    }
}
