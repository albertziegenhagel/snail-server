#pragma once

#include <cassert>

#include <bit>
#include <optional>
#include <span>
#include <string>

namespace snail::common::parser {

// Extract a value of integral type `T` in the native byte order starting at
// the offset `bytes_offset` in the buffer `data` that is in `data_byte_order`.
//
// Precondition: There have to be enough bytes in the buffer.
template<typename T>
    requires std::is_integral_v<T>
T extract(std::span<const std::byte> data, std::size_t bytes_offset, [[maybe_unused]] std::endian data_byte_order)
{
    assert(bytes_offset + sizeof(T) <= data.size());

    auto value = *reinterpret_cast<const T*>(data.data() + bytes_offset);

    if constexpr(sizeof(T) > 1)
    {
        if(data_byte_order != std::endian::native)
        {
            return std::byteswap(value);
        }
        else
        {
            return value;
        }
    }
    else
    {
        return value;
    }
}

// Extract a value of enum type `T` in the native byte order starting at
// the offset `bytes_offset` in the buffer `data` that is in `data_byte_order`.
//
// Precondition: There have to be enough bytes in the buffer.
template<typename T>
    requires std::is_enum_v<T>
T extract(std::span<const std::byte> data, std::size_t bytes_offset, std::endian data_byte_order)
{
    return static_cast<T>(extract<std::underlying_type_t<T>>(data, bytes_offset, data_byte_order));
}

inline std::uint64_t extract_pointer(std::span<const std::byte> data, std::size_t bytes_offset, std::size_t pointer_size, std::endian data_byte_order)
{
    if(pointer_size == 4) return extract<std::uint32_t>(data, bytes_offset, data_byte_order);
    assert(pointer_size == 8);
    return extract<std::uint64_t>(data, bytes_offset, data_byte_order);
}

template<typename T>
    requires std::is_integral_v<T>
inline T extract_move(std::span<const std::byte> buffer, std::size_t& bytes_offset, std::endian byte_order)
{
    const auto value = common::parser::extract<T>(buffer, bytes_offset, byte_order);
    bytes_offset += sizeof(T);
    return value;
}

template<typename T>
    requires std::is_integral_v<T>
inline std::optional<T> extract_move_if(bool condition, std::span<const std::byte> buffer, std::size_t& offset, std::endian byte_order)
{
    if(!condition) return std::nullopt;
    return extract_move<T>(buffer, offset, byte_order);
}

template<typename T>
    requires std::is_integral_v<T>
inline T extract_move_back(std::span<const std::byte> buffer, std::size_t& bytes_offset, std::endian byte_order)
{
    assert(bytes_offset >= sizeof(T));
    bytes_offset -= sizeof(T);
    const auto value = common::parser::extract<T>(buffer, bytes_offset, byte_order);
    return value;
}

template<typename T>
    requires std::is_integral_v<T>
inline std::optional<T> extract_move_back_if(bool condition, std::span<const std::byte> buffer, std::size_t& offset, std::endian byte_order)
{
    if(!condition) return std::nullopt;
    return extract_move_back<T>(buffer, offset, byte_order);
}

template<typename CharIntType>
inline std::size_t detect_string_length(std::span<const std::byte> data, std::size_t bytes_offset, std::endian data_byte_order)
{
    std::size_t size = 0;
    while(size * sizeof(CharIntType) < data.size())
    {
        if(extract<CharIntType>(data, bytes_offset + size * sizeof(CharIntType), data_byte_order) == 0) break;
        ++size;
    }
    assert(size * sizeof(CharIntType) < data.size()); // we could not find a null-terminated string
    return size;
}

struct extract_view_base
{
    inline explicit extract_view_base(std::span<const std::byte> buffer, std::endian byte_order) :
        buffer_(buffer),
        byte_order_(byte_order)
    {}

protected:
    template<typename T>
    inline T extract(std::size_t bytes_offset) const
    {
        return parser::extract<T>(buffer_, bytes_offset, byte_order_);
    }

    inline std::string_view extract_string(std::size_t bytes_offset, std::optional<std::size_t>& string_length) const
    {
        const auto* const chars = reinterpret_cast<const char*>(buffer_.data() + bytes_offset);
        if(!string_length) string_length = detect_string_length<std::uint8_t>(buffer_, bytes_offset, byte_order_);
        return std::string_view(chars, *string_length);
    }

    inline std::u16string_view extract_u16string(std::size_t bytes_offset, std::optional<std::size_t>& string_length) const
    {
        // Just a stupid sanity check. Actually this should never be able to be false, since
        // if the platform provides a std::uint16_t, char16_t has to be 16 bits wide as well
        // and we do not support a platform without std::uint16_t anyways.
        static_assert(sizeof(char16_t) == sizeof(std::uint16_t));

        const auto* const chars = reinterpret_cast<const char16_t*>(buffer_.data() + bytes_offset);
        if(!string_length) string_length = detect_string_length<std::uint16_t>(buffer_, bytes_offset, byte_order_);
        return std::u16string_view(chars, *string_length);
    }

    inline std::span<const std::byte> buffer() const
    {
        return buffer_;
    }
    inline std::endian byte_order() const
    {
        return byte_order_;
    }

private:
    std::span<const std::byte> buffer_;
    std::endian                byte_order_;
};

} // namespace snail::common::parser
