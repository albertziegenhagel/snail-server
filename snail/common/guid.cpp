
#include <snail/common/guid.hpp>

#include <format>
#include <span>

using namespace snail::common;

std::string guid::to_string(bool insert_hyphen) const
{
    std::string result;

    result.reserve(32 + (insert_hyphen ? 4 : 0));

    std::format_to(std::back_inserter(result), "{:08X}", data_1);
    if(insert_hyphen) result.push_back('-');
    std::format_to(std::back_inserter(result), "{:04X}", data_2);
    if(insert_hyphen) result.push_back('-');
    std::format_to(std::back_inserter(result), "{:04X}", data_3);
    if(insert_hyphen) result.push_back('-');

    for(const auto byte : std::span(data_4).subspan(0, 2))
    {
        std::format_to(std::back_inserter(result), "{:02X}", byte);
    }
    if(insert_hyphen) result.push_back('-');
    for(const auto byte : std::span(data_4).subspan(2))
    {
        std::format_to(std::back_inserter(result), "{:02X}", byte);
    }

    return result;
}
