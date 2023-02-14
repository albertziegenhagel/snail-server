#pragma once

#include <snail/jsonrpc/stream/generic/generic_iostream.hpp>

namespace snail::jsonrpc {

class pipe_streambuf;

using pipe_iostream = generic_iostream<pipe_streambuf>;

} // namespace snail::jsonrpc
