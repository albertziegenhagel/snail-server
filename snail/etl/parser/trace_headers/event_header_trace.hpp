
#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/etl/parser/trace.hpp>
#include <snail/etl/parser/utility.hpp>
#include <snail/etl/parser/extract.hpp>

namespace snail::etl::parser {

// See EVENT_HEADER_FLAG_* in evntcons.h
enum class event_header_flag : std::uint16_t
{
    extended_info   = 1,
    private_session = 2,
    string_only     = 4,
    trace_message   = 8,
    no_cputime      = 16,
    header_32_bit   = 32,
    header_64_bit   = 64,
    classic_header  = 256,
    processor_index = 512
};

// See EVENT_HEADER_PROPERTY_* in evntcons.h
enum class event_header_property_flag : std::uint16_t
{
    xml             = 1,
    forwarded_xml   = 2,
    legacy_eventlog = 4,
    reloggable      = 8
};

// See _EVENT_DESCRIPTOR in evntprov.h
struct event_descriptor_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto id() const { return extract<std::uint16_t>(0); }
    inline auto version() const { return extract<std::uint8_t>(2); }
    inline auto channel() const { return extract<std::uint8_t>(3); }
    inline auto level() const { return extract<std::uint8_t>(4); }
    inline auto opcode() const { return extract<std::uint8_t>(5); }
    inline auto task() const { return extract<std::uint16_t>(6); }
    inline auto keyword() const { return extract<std::uint64_t>(8); }

    static inline constexpr std::size_t static_size = 16;
};

// See EVENT_HEADER in evntcons.h
// the head structure (size, header_type, header_flags) has been adjusted
struct event_header_trace_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    using extract_view_base::buffer;
    
    inline auto size() const { return extract<std::uint16_t>(0); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }

    inline auto flags() const { return extract<std::underlying_type_t<event_header_flag>>(4); }
    inline auto event_property() const { return extract<std::underlying_type_t<event_header_property_flag>>(6); }

    inline auto thread_id() const { return extract<std::uint32_t>(8); }
    inline auto process_id() const { return extract<std::uint32_t>(12); }

    inline auto timestamp() const { return extract<std::uint64_t>(16); }

    inline auto provider_id() const { return guid_view(buffer().subspan(24)); }

    inline auto event_descriptor() const { return event_descriptor_view(buffer().subspan(24 + guid_view::static_size)); }

    inline auto processor_time() const { return extract<std::uint64_t>(24 + guid_view::static_size + event_descriptor_view::static_size); }

    inline auto activity_id() const { return guid_view(buffer().subspan(32 + guid_view::static_size + event_descriptor_view::static_size)); }

    static inline constexpr std::size_t static_size = 32 + 2*guid_view::static_size + event_descriptor_view::static_size;
};

} // namespace snail::etl::parser
