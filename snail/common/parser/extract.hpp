#pragma once

#include <cassert>
#include <cstdint>

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

template<typename T>
    requires std::is_same_v<T, float>
T extract(std::span<const std::byte> data, std::size_t bytes_offset, std::endian data_byte_order)
{
    return std::bit_cast<T>(extract<std::uint32_t>(data, bytes_offset, data_byte_order));
}

template<typename T>
    requires std::is_same_v<T, double>
T extract(std::span<const std::byte> data, std::size_t bytes_offset, std::endian data_byte_order)
{
    return std::bit_cast<T>(extract<std::uint64_t>(data, bytes_offset, data_byte_order));
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

inline bool is_all_zero(std::span<const std::byte> data, std::size_t bytes_offset, std::size_t bytes_size)
{
    for(const auto byte : data.subspan(bytes_offset, bytes_size))
    {
        if(byte != std::byte{0}) return false;
    }
    return true;
}

template<typename CharType>
inline std::size_t detect_string_size(std::span<const std::byte> data, std::size_t bytes_offset)
{
    std::size_t next_char_bytes_offset = bytes_offset;
    while(next_char_bytes_offset < data.size())
    {
        // Check for null termination
        if(is_all_zero(data, next_char_bytes_offset, sizeof(CharType))) break;
        next_char_bytes_offset += sizeof(CharType);
    }
    assert(next_char_bytes_offset < data.size()); // we could not find a null-terminated string
    return (next_char_bytes_offset - bytes_offset) / sizeof(CharType);
}

template<typename CharType>
inline std::basic_string_view<CharType> extract_string(std::span<const std::byte> data, std::size_t bytes_offset, std::optional<std::size_t>& string_size)
{
    if(!string_size) string_size = detect_string_size<CharType>(data, bytes_offset);

    // NOTE: we do not swap bytes here, even if sizeof(CharType) > 0 (e.g. UTF-16).
    //       This is because we assume that UTF-16 strings are always encoded in little endian,
    //       even on machines that represent integers in big endian byte order.
    const auto* const chars = reinterpret_cast<const CharType*>(data.data() + bytes_offset);
    return std::basic_string_view<CharType>(chars, *string_size);
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
        return parser::extract_string<char>(buffer_, bytes_offset, string_length);
    }

    inline std::u16string_view extract_u16string(std::size_t bytes_offset, std::optional<std::size_t>& string_length) const
    {
        return parser::extract_string<char16_t>(buffer_, bytes_offset, string_length);
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
