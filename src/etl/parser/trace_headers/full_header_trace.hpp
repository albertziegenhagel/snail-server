
#pragma once

#include <cstdint>

#include <type_traits>

#include "etl/parser/trace.hpp"
#include "etl/parser/utility.hpp"
#include "etl/parser/extract.hpp"

#include "etl/parser/trace_headers/common.hpp"

namespace perfreader::etl::parser {

// See EVENT_TRACE_HEADER in evntrace.h
struct full_header_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    using extract_view_base::buffer;
    
    inline auto size() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }
    
    inline auto trace_class() const { return trace_class_view(buffer().subspan(4)); }

    inline auto thread_id() const { return extract<std::uint32_t>(4 + trace_class_view::static_size); }
    inline auto process_id() const { return extract<std::uint32_t>(8 + trace_class_view::static_size); }

    inline auto timestamp() const { return extract<std::uint64_t>(12 + trace_class_view::static_size); }

    inline auto guid() const { return guid_view(buffer().subspan(20 + trace_class_view::static_size)); }

    // TODO: determine how to distinguish whether to use `processor_time` or `kernel_time` and `user_time`
    inline auto processor_time() const { return extract<std::uint64_t>(20 + trace_class_view::static_size + guid_view::static_size); }
    inline auto kernel_time() const { return extract<std::uint32_t>(20 + trace_class_view::static_size + guid_view::static_size); }
    inline auto user_time() const { return extract<std::uint32_t>(24 + trace_class_view::static_size + guid_view::static_size); }

    static inline constexpr std::size_t static_size = 28 + trace_class_view::static_size + guid_view::static_size;
};

} // namespace perfreader::etl::parser
