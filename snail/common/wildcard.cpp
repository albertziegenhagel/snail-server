
#include <snail/common/wildcard.hpp>

#include <vector>

using namespace snail::common;

void append_regex_escaped(std::string& output, char c)
{
    static constexpr std::string_view special_chars = "()[]{}?*+-|^$\\.&~# \t\n\r\v\f";
    if(special_chars.contains(c)) output.push_back('\\');
    output.push_back(c);
}

std::string snail::common::wildcard_to_regex(std::string_view pattern)
{
    std::string result = "^(?:";

    bool last_was_star = false;

    const auto size = pattern.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        const auto c = pattern[i];
        if(c == '*')
        {
            if(last_was_star) continue; // multiple consecutive stars are handled as one.
            result += ".*";
            last_was_star = true;
        }
        else if(c == '?')
        {
            result.push_back('.');
            last_was_star = false;
        }
        else
        {
            append_regex_escaped(result, c);
            last_was_star = false;
        }
    }
    result += ")$";
    return result;
}
