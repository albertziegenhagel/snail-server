
#include <snail/jsonrpc/jsonrpc_v2_protocol.hpp>

#include <format>

#include <snail/jsonrpc/errors.hpp>

using namespace snail::jsonrpc;

request v2_protocol::load_request(std::string_view content)
{
    nlohmann::json data;
    try
    {
        data = nlohmann::json::parse(content);
    }
    catch(const nlohmann::json::parse_error& e)
    {
        throw parse_error(e.what());
    }

    if(!data.is_object()) throw invalid_request_error("Root is not an object");

    const auto jsonrpc = data.find("jsonrpc");
    if(jsonrpc == data.end()) throw invalid_request_error("'jsonrpc' missing");
    const auto* const jsonrpc_str = jsonrpc->get<const std::string*>();
    if(jsonrpc_str == nullptr) throw invalid_request_error("'jsonrpc' is not a string");
    if(*jsonrpc_str != "2.0") throw invalid_request_error("'jsonrpc' has to '2.0'");

    const auto method = data.find("method");
    if(method == data.end()) throw invalid_request_error("'method' missing");
    auto* const method_str = method->get<std::string*>();
    if(method_str == nullptr) throw invalid_request_error("'method' is not a string");

    nlohmann::json params_data;
    const auto     params = data.find("params");
    if(params != data.end())
    {
        if(!params->is_object() && !params->is_array()) throw invalid_request_error("'params' is neither and object, nor an array.");
        params->swap(params_data);
    }

    std::optional<nlohmann::json> id_data;
    const auto                    id = data.find("id");
    if(id != data.end())
    {
        if(!id->is_string() && !id->is_number() && !id->is_null()) throw invalid_request_error("'id' is neither and string, nor number, nor null.");
        id_data.emplace();
        id->swap(*id_data);
    }

    const auto expected_count = std::size_t(2) + // jsonrpc & method
                                (params == data.end() ? std::size_t(0) : std::size_t(1)) +
                                (id == data.end() ? std::size_t(0) : std::size_t(1));
    if(data.size() != expected_count) throw invalid_request_error("to many fields found.");

    return request{
        .method = std::move(*method_str),
        .params = std::move(params_data),
        .id     = std::move(id_data)};
}

std::string v2_protocol::dump_response(const jsonrpc::response& response)
{
    if(response.id)
    {
        return std::format(R"({{"jsonrpc":"2.0","result":{},"id":{}}})", response.result.dump(), response.id->dump());
    }

    return std::format(R"({{"jsonrpc":"2.0","result":{}}})", response.result.dump());
}

std::string v2_protocol::dump_error(const rpc_error& error, const nlohmann::json* id)
{
    const auto json_error = nlohmann::json{
        {"code",    error.code()},
        {"message", error.what()}
    };

    if(id != nullptr)
    {
        return std::format(R"({{"jsonrpc":"2.0","error":{},"id":{}}})", json_error.dump(), id->dump());
    }

    return std::format(R"({{"jsonrpc":"2.0","error":{}}})", json_error.dump());
}
