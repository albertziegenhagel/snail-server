
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"
#include "etl/parser/utility.hpp"
#include "etl/parser/extract.hpp"

namespace perfreader::etl::parser {

struct trace_class
{
    std::uint8_t type;
    std::uint8_t level;
    std::uint16_t version;
};

std::size_t parse(utility::binary_stream_parser& stream, trace_class& data);

struct trace_class_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto type() const { return extract<std::uint8_t>(0); }
    inline auto level() const { return extract<std::uint8_t>(1); }
    inline auto version() const { return extract<std::uint16_t>(2); }

    static inline constexpr std::size_t static_size = 4;
};

// See EVENT_TRACE_HEADER in evntrace.h
struct full_header_trace_header
{
    std::uint16_t size;
    trace_header_type header_type;
    std::uint8_t  header_flags;

    trace_class class_;

    std::uint32_t thread_id;
    std::uint32_t process_id;

    std::uint64_t timestamp;
    parser::guid guid;
    std::uint64_t processor_time;
};

std::size_t parse(utility::binary_stream_parser& stream, full_header_trace_header& data);

struct full_header_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto size() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }
    
    inline auto trace_class() const { return trace_class_view(buffer_.subspan(4)); }

    inline auto thread_id() const { return extract<std::uint32_t>(4 + trace_class_view::static_size); }
    inline auto process_id() const { return extract<std::uint32_t>(8 + trace_class_view::static_size); }

    inline auto timestamp() const { return extract<std::uint64_t>(12 + trace_class_view::static_size); }
    inline auto guid() const { return guid_view(buffer_.subspan(20 + trace_class_view::static_size)); }
    inline auto processor_time() const { return extract<std::uint64_t>(20 + trace_class_view::static_size + guid_view::static_size); }

    static inline constexpr std::size_t static_size = 28 + trace_class_view::static_size + guid_view::static_size;
};

} // namespace perfreader::etl::parser
