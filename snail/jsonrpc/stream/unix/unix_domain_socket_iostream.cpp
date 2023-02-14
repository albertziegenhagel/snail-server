
#include <snail/jsonrpc/stream/unix/unix_domain_socket_iostream.hpp>

#include <snail/jsonrpc/stream/unix/unix_domain_socket_streambuf.hpp>

#include <snail/jsonrpc/stream/generic/generic_iostream_impl.hpp>

template class snail::jsonrpc::generic_iostream<snail::jsonrpc::unix_domain_socket_streambuf>;
