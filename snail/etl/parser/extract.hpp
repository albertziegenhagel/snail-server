#pragma once

#include <cassert>

#include <bit>
#include <optional>
#include <span>
#include <string>

#include <snail/common/parser/extract.hpp>

namespace snail::etl::parser {

// ETL files are written on Windows, hence they are always little-endian.
inline constexpr auto etl_file_byte_order = std::endian::little;

// This is basically just the `extract_view_base` from the `common::parser`
// with the byte order assumed to be `etl_file_byte_order`.
struct extract_view_base : protected common::parser::extract_view_base
{
    inline explicit extract_view_base(std::span<const std::byte> buffer) :
        common::parser::extract_view_base(buffer, etl_file_byte_order)
    {}
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
        return common::parser::extract_pointer(buffer(), bytes_offset, pointer_size_, byte_order());
    }

    inline std::uint32_t pointer_size() const
    {
        return pointer_size_;
    }

private:
    std::uint32_t pointer_size_;
};

} // namespace snail::etl::parser
