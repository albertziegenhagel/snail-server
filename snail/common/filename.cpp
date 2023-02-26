
#include <snail/common/filename.hpp>

#include <random>

using namespace snail::common;

std::string snail::common::make_random_filename(std::size_t length)
{
    static constexpr const char characters[]   = "0123456789abcdefghijklmnopqrstuvwxyz";
    static constexpr auto       num_characters = sizeof(characters) - 1; // -1 for null-termination

    std::random_device rd;
    std::mt19937       generator(rd());

    std::uniform_int_distribution<std::size_t> distribution(0, num_characters - 1); // -1 because std::uniform_int_distribution creates numbers in a closed interval.

    std::string result;
    result.reserve(length);
    for(std::size_t i = 0; i < length; ++i)
    {
        result.push_back(characters[distribution(generator)]);
    }

    return result;
}
