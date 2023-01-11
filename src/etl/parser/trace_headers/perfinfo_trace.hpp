
#pragma once

#include <cstdint>

#include <type_traits>

#include "etl/parser/trace.hpp"
#include "etl/parser/extract.hpp"

namespace perfreader::etl::parser {

// See _PERFINFO_TRACE_HEADER in ntwmi.h
struct perfinfo_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto version() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }
    
    inline auto packet() const { return wmi_trace_packet_view(buffer_.subspan(4)); }
    
    inline auto system_time() const { return extract<std::uint64_t>(4 + wmi_trace_packet_view::static_size); }

    static inline constexpr std::size_t static_size = 12 + wmi_trace_packet_view::static_size;
};

} // namespace perfreader::etl::parser
