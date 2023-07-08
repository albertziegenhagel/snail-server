#pragma once

#include <filesystem>
#include <string_view>

namespace snail::common {

std::filesystem::path path_from_utf8(std::string_view path_str);

} // namespace snail::common
