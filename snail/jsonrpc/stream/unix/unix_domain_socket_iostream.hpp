#pragma once

#include <snail/jsonrpc/stream/generic/generic_iostream.hpp>

namespace snail::jsonrpc {

class unix_domain_socket_streambuf;

using unix_domain_socket_iostream = generic_iostream<unix_domain_socket_streambuf>;

} // namespace snail::jsonrpc
