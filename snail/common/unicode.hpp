#pragma once

#include <string>

namespace snail::common {

template<typename Utf8Char>
    requires std::is_same_v<Utf8Char, char> || std::is_same_v<Utf8Char, char8_t>
std::basic_string<Utf8Char> utf16_to_utf8(std::u16string_view input);

} // namespace snail::common
