
#pragma once

#include <cstdint>

#include <array>
#include <functional>
#include <string>

#include <snail/common/hash_combine.hpp>

namespace snail::common {

struct guid
{
    std::uint32_t               data_1;
    std::uint16_t               data_2;
    std::uint16_t               data_3;
    std::array<std::uint8_t, 8> data_4;

    std::string to_string(bool insert_hyphen = false) const;

    [[nodiscard]] constexpr friend bool operator==(const guid& lhs, const guid& rhs) noexcept
    {
        return lhs.data_1 == rhs.data_1 &&
               lhs.data_2 == rhs.data_2 &&
               lhs.data_3 == rhs.data_3 &&
               lhs.data_4 == rhs.data_4;
    }
};

} // namespace snail::common

namespace std {

template<>
struct hash<snail::common::guid>
{
    size_t operator()(const snail::common::guid& guid) const noexcept
    {
        const std::uint64_t*     p = reinterpret_cast<const std::uint64_t*>(&guid);
        std::hash<std::uint64_t> hash;
        return snail::common::hash_combine(hash(p[0]), hash(p[1]));
    }
};

} // namespace std
