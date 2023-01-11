
#pragma once

#include <cstdint>

#include <type_traits>

#include "etl/parser/trace.hpp"
#include "etl/parser/utility.hpp"
#include "etl/parser/extract.hpp"

namespace perfreader::etl::parser {

// See _EVENT_INSTANCE_GUID_HEADER in ntwmi.h
struct instance_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto size() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }

    // could be `trace_class` as well
    inline auto version() const { return extract<std::uint64_t>(4); }

    inline auto thread_id() const { return extract<std::uint32_t>(12); }
    inline auto process_id() const { return extract<std::uint32_t>(16); }

    inline auto timestamp() const { return extract<std::uint64_t>(20); }

    inline auto guid() const { return guid_view(buffer_.subspan(28)); }

     // kernel_time + user_time could be `timestamp` as well
    inline auto kernel_time() const { return extract<std::uint32_t>(28 + guid_view::static_size); }
    inline auto user_time() const { return extract<std::uint32_t>(32 + guid_view::static_size); }

    inline auto instance_id() const { return extract<std::uint32_t>(36 + guid_view::static_size); }
    inline auto parent_instance_id() const { return extract<std::uint32_t>(40 + guid_view::static_size); }

    inline auto parent_guid() const { return guid_view(buffer_.subspan(44)); }

    static inline constexpr std::size_t static_size = 44 + 2*guid_view::static_size;
};

} // namespace perfreader::etl::parser
