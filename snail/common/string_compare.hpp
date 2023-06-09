#pragma once

#include <string_view>

namespace snail::common {

// case insensitive compare
bool ascii_iequals(std::string_view lhs, std::string_view rhs);

} // namespace snail::common
