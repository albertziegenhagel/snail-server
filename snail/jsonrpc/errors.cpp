#include <snail/jsonrpc/errors.hpp>

using namespace snail::jsonrpc;

rpc_error::rpc_error(int code, const char* message) :
    std::runtime_error(message),
    code_(code)
{}

int rpc_error::code() const
{
    return code_;
}

parse_error::parse_error(const char* message) :
    rpc_error(-32700, message)
{}

invalid_request_error::invalid_request_error(const char* message) :
    rpc_error(-32600, message)
{}

unknown_method_error::unknown_method_error(const char* message) :
    rpc_error(-32601, message)
{}

invalid_parameters_error::invalid_parameters_error(const char* message) :
    rpc_error(-32602, message)
{}

internal_error::internal_error(const char* message) :
    rpc_error(-32603, message)
{}

transport_error::transport_error(const char* message) :
    rpc_error(-32000, message)
{}
