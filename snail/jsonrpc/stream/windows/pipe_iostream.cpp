
#include <snail/jsonrpc/stream/windows/pipe_iostream.hpp>

#include <snail/jsonrpc/stream/windows/pipe_streambuf.hpp>

#include <snail/jsonrpc/stream/generic/generic_iostream_impl.hpp>

template class snail::jsonrpc::generic_iostream<snail::jsonrpc::pipe_streambuf>;
