#pragma once

#include <string>
#include <string_view>

namespace snail::common {

std::string wildcard_to_regex(std::string_view pattern);

} // namespace snail::common
