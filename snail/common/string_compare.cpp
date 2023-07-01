
#include <snail/common/string_compare.hpp>

using namespace snail::common;

namespace {

char ascii_tolower(char ch)
{
    const auto in = static_cast<unsigned char>(ch);
    return static_cast<char>(in > 96 && in < 123 ? (in - 32) : in);
}

} // namespace

bool snail::common::ascii_iequals(std::string_view lhs, std::string_view rhs)
{
    if(lhs.size() != rhs.size()) return false;
    for(std::size_t i = 0; i < lhs.size(); ++i)
    {
        if(ascii_tolower(lhs[i]) != ascii_tolower(rhs[i])) return false;
    }
    return true;
}
