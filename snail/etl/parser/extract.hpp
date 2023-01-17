#pragma once

#include <cassert>

#include <bit>
#include <span>
#include <optional>
#include <string>

namespace snail::etl::parser {

// ETL files are written on Windows, hence they are always little-endian.
inline constexpr auto etl_file_byte_order = std::endian::little;

// Extract a value of integral type `T` in the native byte order starting at
// the offset `bytes_offset` in the buffer `data` that is in `etl_file_byte_order`.
//
// Precondition: There have to be enough bytes in the buffer.
template<typename T>
    requires std::is_integral_v<T>
T extract(std::span<const std::byte> data, std::size_t bytes_offset, std::endian data_byte_order)
{
    assert(bytes_offset + sizeof(T) <= data.size());

    auto value = *reinterpret_cast<const T*>(data.data() + bytes_offset);
    if(data_byte_order != std::endian::native)
    {
        return std::byteswap(value);
    }
    else
    {
        return value;
    }
}

template<typename T>
    requires std::is_integral_v<T>
T extract(std::span<const std::byte> data, std::size_t bytes_offset)
{
    return extract<T>(data, bytes_offset, etl_file_byte_order);
}

// Extract a value of enum type `T` in the native byte order starting at
// the offset `bytes_offset` in the buffer `data` that is in `etl_file_byte_order`.
//
// Precondition: There have to be enough bytes in the buffer.
template<typename T>
    requires std::is_enum_v<T>
T extract(std::span<const std::byte> data, std::size_t bytes_offset)
{
    return static_cast<T>(extract<std::underlying_type_t<T>>(data, bytes_offset));
}

template<typename CharIntType>
inline std::size_t detect_string_length(std::span<const std::byte> data, std::size_t bytes_offset)
{
    std::size_t size = 0;
    while(size*sizeof(CharIntType) < data.size())
    {
        if(extract<CharIntType>(data, bytes_offset + size*sizeof(CharIntType)) == 0) break;
        ++size;
    }
    assert(size*sizeof(CharIntType) < data.size()); // we could not find a null-terminated string
    return size;
}

inline std::uint64_t extract_pointer(std::span<const std::byte> data, std::size_t bytes_offset, std::size_t pointer_size)
{
    if(pointer_size == 4) return extract<std::uint32_t>(data, bytes_offset);
    assert(pointer_size == 8);
    return extract<std::uint64_t>(data, bytes_offset);
}

struct extract_view_base
{
    inline explicit extract_view_base(std::span<const std::byte> buffer) :
        buffer_(buffer)
    {}

protected:
    template<typename T>
    inline T extract(std::size_t bytes_offset) const
    {
        return parser::extract<T>(buffer_, bytes_offset);
    }

    inline std::string_view extract_string(std::size_t bytes_offset, std::optional<std::size_t>& string_length) const
    {
        const auto* const chars = reinterpret_cast<const char*>(buffer_.data() + bytes_offset);
        if(!string_length) string_length = detect_string_length<std::uint8_t>(buffer_, bytes_offset);
        return std::string_view(chars, *string_length);
    }
    
    inline std::u16string_view extract_u16string(std::size_t bytes_offset, std::optional<std::size_t>& string_length) const
    {
        // Just a stupid sanity check. Actually this should never be able to be false, since
        // if the platform provides a std::uint16_t, char16_t has to be 16 bits wide as well
        // and we do not support a platform without std::uint16_t anyways.
        static_assert(sizeof(char16_t) == sizeof(std::uint16_t));

        const auto* const chars = reinterpret_cast<const char16_t*>(buffer_.data() + bytes_offset);
        if(!string_length) string_length = detect_string_length<std::uint16_t>(buffer_, bytes_offset);
        return std::u16string_view(chars, *string_length);
    }

    inline std::span<const std::byte> buffer() const
    {
        return buffer_;
    }

private:
    std::span<const std::byte> buffer_;
};


struct extract_view_dynamic_base : protected extract_view_base
{
    inline explicit extract_view_dynamic_base(std::span<const std::byte> buffer, std::uint32_t pointer_size) :
        extract_view_base(buffer),
        pointer_size_(pointer_size)
    {}

protected:

    // Computes the offset of a member within a record depending on the used pointer_size.
    //
    // @param previous_bytes_count   The size in bytes of all non-pointer values before the value to compute the offset of.
    // @param previous_pointer_count The number of pointer values before the value to compute the offset of.
    constexpr inline std::size_t dynamic_offset(std::size_t bytes_offset, std::size_t pointer_count) const
    {
        return bytes_offset + pointer_count * pointer_size_;
    }

    inline std::uint64_t extract_pointer(std::size_t bytes_offset) const
    {
        return parser::extract_pointer(buffer(), bytes_offset, pointer_size_);
    }
    
    inline std::uint32_t pointer_size() const
    {
        return pointer_size_;
    }
private:
    std::uint32_t pointer_size_;
};

} // snail::etl::parser