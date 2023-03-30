
#include <snail/common/trim.hpp>

#include <cctype>

#include <algorithm>
#include <ranges>

using namespace snail::common;

std::string_view snail::common::trim_left(std::string_view input)
{
    const auto first_nospace_iter = std::ranges::find_if(input,
                                                         [](int c)
                                                         { return std::isspace(c) == 0; });
    input.remove_prefix(std::distance(input.cbegin(), first_nospace_iter));
    return input;
}

std::string_view snail::common::trim_right(std::string_view input)
{
    const auto first_reversed_nospace_iter = std::ranges::find_if(input | std::views::reverse,
                                                                  [](int c)
                                                                  { return std::isspace(c) == 0; });
    input.remove_suffix(std::distance(input.crbegin(), first_reversed_nospace_iter));
    return input;
}

std::string_view snail::common::trim(std::string_view input)
{
    return trim_left(trim_right(input));
}
