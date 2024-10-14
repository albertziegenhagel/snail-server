#pragma once

#include <memory>

#include <snail/jsonrpc/server.hpp>

#include <snail/server/detail/storage.hpp>

#include <snail/common/thread_pool.hpp>

namespace snail::server {

class snail_server
{
public:
    explicit snail_server(std::unique_ptr<jsonrpc::message_connection> connection,
                          std::unique_ptr<jsonrpc::protocol>           protocol,
                          std::unique_ptr<common::thread_pool>         thread_pool);
    ~snail_server();

    bool serve();

private:
    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::server
