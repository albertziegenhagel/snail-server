#pragma once

#include <snail/jsonrpc/server.hpp>

namespace snail::server {

class storage;

void register_all(snail::jsonrpc::server& server, server::storage& storage);

} // namespace snail::server
