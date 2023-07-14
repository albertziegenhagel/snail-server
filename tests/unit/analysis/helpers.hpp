#include <bit>
#include <ranges>
#include <span>
#include <string_view>

#include <snail/common/guid.hpp>

namespace snail::detail::tests {

template<typename T>
inline void set_at(std::span<std::byte> buffer, std::size_t bytes_offset, T value)
{
    auto& destination = *reinterpret_cast<T*>(buffer.data() + bytes_offset);

    if constexpr(std::endian::native != std::endian::little)
    {
        destination = std::byteswap(value);
    }
    else
    {
        destination = value;
    }
}

inline void set_at(std::span<std::byte> buffer, std::size_t bytes_offset, std::string_view value)
{
    auto* destination = reinterpret_cast<char*>(buffer.data() + bytes_offset);
    std::copy(value.data(), value.data() + value.size(), destination);
    destination[value.size()] = '\0';
}

inline void set_at(std::span<std::byte> buffer, std::size_t bytes_offset, std::u16string_view value)
{
    auto* destination = reinterpret_cast<char16_t*>(buffer.data() + bytes_offset);
    std::copy(value.data(), value.data() + value.size(), destination);
    destination[value.size()] = '\0';
}

inline void set_at(std::span<std::byte> buffer, std::size_t bytes_offset, const common::guid& value)
{
    set_at(buffer, bytes_offset, value.data_1);
    set_at(buffer, bytes_offset + 4, value.data_2);
    set_at(buffer, bytes_offset + 6, value.data_3);
    std::ranges::copy(std::as_bytes(std::span(value.data_4)), buffer.begin() + bytes_offset + 8);
}

} // namespace snail::detail::tests
