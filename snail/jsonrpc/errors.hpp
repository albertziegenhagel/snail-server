#pragma once

#include <stdexcept>

namespace snail::jsonrpc {

class rpc_error : public std::runtime_error
{
public:
    explicit rpc_error(int code, const char* message);

    int code() const;

private:
    int code_;
};

class parse_error : public rpc_error
{
public:
    explicit parse_error(const char* message);
};

class invalid_request_error : public rpc_error
{
public:
    explicit invalid_request_error(const char* message);
};

class unknown_method_error : public rpc_error
{
public:
    explicit unknown_method_error(const char* message);
};

class invalid_parameters_error : public rpc_error
{
public:
    explicit invalid_parameters_error(const char* message);
};

class internal_error : public rpc_error
{
public:
    explicit internal_error(const char* message);
};

class transport_error : public rpc_error
{
public:
    explicit transport_error(const char* message);
};

} // namespace snail::jsonrpc
