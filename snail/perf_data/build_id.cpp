#include <snail/perf_data/build_id.hpp>

#include <cstdint>
#include <format>

using namespace snail::perf_data;

std::string build_id::to_string() const
{
    std::string result;
    result.reserve(size_ * 2);

    for(std::size_t i = 0; i < size_; ++i)
    {
        std::format_to(std::back_inserter(result), "{:02x}", std::to_integer<std::uint8_t>(buffer_[i]));
    }
    return result;
}
