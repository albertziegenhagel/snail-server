
#pragma once

#include <cstdint>

#include <array>
#include <functional>

namespace snail::etl {

// See GUID from guiddef.h
struct guid
{
    std::uint32_t               data_1;
    std::uint16_t               data_2;
    std::uint16_t               data_3;
    std::array<std::uint8_t, 8> data_4;

    bool operator==(const guid& other) const noexcept
    {
        return data_1 == other.data_1 &&
            data_2 == other.data_2 &&
            data_3 == other.data_3 &&
            data_4 == other.data_4;
    }
};

} // namespace snail::etl


namespace std {

template<>
struct hash<snail::etl::guid>
{
    size_t operator()(const snail::etl::guid& guid) const noexcept
    {
        const std::uint64_t* p = reinterpret_cast<const std::uint64_t*>(&guid);
        std::hash<std::uint64_t> hash;
        return hash(p[0]) ^ hash(p[1]);
    }
};

} // namespace std