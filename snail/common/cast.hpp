#pragma once

#include <concepts>

namespace snail::common {

template<std::integral U, std::integral T>
constexpr U narrow_cast(T u) noexcept
{
    return static_cast<U>(u);
}

} // namespace snail::common
