
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"
#include "etl/parser/extract.hpp"

namespace perfreader::etl::parser {

// See _SYSTEM_TRACE_HEADER in ntwmi.h
// but only up to SystemTime (missing KernelTime and UserTime)
struct compact_trace_header
{
    std::uint16_t version;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    wmi_trace_packet packet;

    std::uint32_t thread_id;
    std::uint32_t process_id;

    std::uint64_t system_time;
};

std::size_t parse(utility::binary_stream_parser& stream, compact_trace_header& data);

struct compact_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto version() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }
    
    inline auto packet() const { return wmi_trace_packet_view(buffer_.subspan(4)); }
    
    inline auto thread_id() const { return extract<std::uint32_t>(4 + wmi_trace_packet_view::static_size); }
    inline auto process_id() const { return extract<std::uint32_t>(8 + wmi_trace_packet_view::static_size); }
    
    inline auto system_time() const { return extract<std::uint64_t>(12 + wmi_trace_packet_view::static_size); }

    static inline constexpr std::size_t static_size = 20 + wmi_trace_packet_view::static_size;
};

} // namespace perfreader::etl::parser
