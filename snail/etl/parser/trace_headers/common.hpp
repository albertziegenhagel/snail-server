
#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/etl/parser/extract.hpp>

namespace snail::etl::parser {

struct trace_class_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto type() const { return extract<std::uint8_t>(0); }
    inline auto level() const { return extract<std::uint8_t>(1); }
    inline auto version() const { return extract<std::uint16_t>(2); }

    static inline constexpr std::size_t static_size = 4;
};

} // namespace snail::etl::parser
