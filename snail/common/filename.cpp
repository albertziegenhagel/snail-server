
#include <snail/common/filename.hpp>

#include <array>
#include <random>

using namespace snail::common;

std::string snail::common::make_random_filename(std::size_t length)
{
    static constexpr const auto characters = std::array{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'};

    std::random_device rd;
    std::mt19937       generator(rd());

    std::uniform_int_distribution<std::size_t> distribution(0, characters.size() - 1); // -1 because std::uniform_int_distribution creates numbers in a closed interval.

    std::string result;
    result.reserve(length);
    for(std::size_t i = 0; i < length; ++i)
    {
        result.push_back(characters[distribution(generator)]);
    }

    return result;
}
