#pragma once

#include <string_view>

namespace snail::common {

std::string_view trim_left(std::string_view input);

std::string_view trim_right(std::string_view input);

std::string_view trim(std::string_view input);

} // namespace snail::common
