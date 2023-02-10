#pragma once

#include <string>

namespace snail::analysis {

struct system_info
{
    std::string hostname;
    std::string platform;
    std::string architecture;
    std::string cpu_name;
    std::size_t number_of_processors;
};

} // namespace snail::analysis
