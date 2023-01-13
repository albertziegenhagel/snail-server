
#pragma once

#include <cstdint>

#include <type_traits>

#include "etl/parser/trace.hpp"
#include "etl/parser/extract.hpp"

namespace snail::etl::parser {

// See _SYSTEM_TRACE_HEADER in ntwmi.h
struct system_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    using extract_view_base::buffer;
    
    inline auto version() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }
    
    inline auto packet() const { return wmi_trace_packet_view(buffer().subspan(4)); }
    
    inline auto thread_id() const { return extract<std::uint32_t>(4 + wmi_trace_packet_view::static_size); }
    inline auto process_id() const { return extract<std::uint32_t>(8 + wmi_trace_packet_view::static_size); }
    
    inline auto system_time() const { return extract<std::uint64_t>(12 + wmi_trace_packet_view::static_size); }
    inline auto kernel_time() const { return extract<std::uint32_t>(20 + wmi_trace_packet_view::static_size); }
    inline auto user_time() const { return extract<std::uint32_t>(24 + wmi_trace_packet_view::static_size); }

    static inline constexpr std::size_t static_size = 28 + wmi_trace_packet_view::static_size;
};

} // namespace snail::etl::parser
