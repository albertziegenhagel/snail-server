#pragma once

#include <type_traits>

namespace snail::common {

inline constexpr std::size_t hash_combine(std::size_t lhs, std::size_t rhs) noexcept
{
    return lhs ^ rhs;
}

} // namespace snail::common
