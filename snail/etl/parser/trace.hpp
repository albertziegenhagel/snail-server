
#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/etl/parser/extract.hpp>

namespace snail::etl::parser {

// See TRACE_HEADER_TYPE_* from ntwmi.h
enum class trace_header_type : std::uint8_t
{
    system32       = 1,
    system64       = 2,
    compact32      = 3,
    compact64      = 4,
    full_header32  = 10,
    instance32     = 11,
    timed          = 12, // not used
    error          = 13, // error while logging event
    wnode_header   = 14, // not used
    message        = 15,
    perfinfo32     = 16,
    perfinfo64     = 17,
    event_header32 = 18,
    event_header64 = 19,
    full_header64  = 20,
    instance64     = 21
};

// See EVENT_TRACE_GROUP_* from ntwmi.h
enum class event_trace_group : std::uint8_t
{
    header      = 0,
    io          = 1,
    memory      = 2,
    process     = 3,
    file        = 4,
    thread      = 5,
    tcpip       = 6,
    job         = 7,
    udpip       = 8,
    registry    = 9,
    dbgprint    = 10,
    config      = 11,
    spare1      = 12,
    wnf         = 13,
    pool        = 14,
    perfinfo    = 15,
    heap        = 16,
    object      = 17,
    power       = 18,
    modbound    = 19,
    image       = 20,
    dpc         = 21,
    cc          = 22,
    critsec     = 23,
    stackwalk   = 24,
    ums         = 25,
    alpc        = 26,
    splitio     = 27,
    thread_pool = 28,
    hypervisor  = 29,
    hypervisorx = 30
};

struct generic_trace_marker
{
    std::uint16_t     reserved; // usually either size or version
    trace_header_type header_type;
    std::uint8_t      header_flags;

    static constexpr inline std::uint32_t trace_header_flag             = 0x80; // shifted `TRACE_HEADER_FLAG` from ntwmi.h
    static constexpr inline std::uint32_t trace_header_event_trace_flag = 0x40; // shifted `TRACE_HEADER_EVENT_TRACE` from ntwmi.h
    static constexpr inline std::uint32_t trace_message_flag            = 0x10; // shifted `TRACE_MESSAGE` from ntwmi.h

    bool is_trace_header() const;
    bool is_trace_header_event_trace() const;
    bool is_trace_message() const;
};

struct generic_trace_marker_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto reserved() const { return extract<std::uint16_t>(0); } // usually either size or version
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }

    static inline constexpr std::size_t static_size = 4;

    static constexpr inline std::uint32_t trace_header_flag             = 0x80; // shifted `TRACE_HEADER_FLAG` from ntwmi.h
    static constexpr inline std::uint32_t trace_header_event_trace_flag = 0x40; // shifted `TRACE_HEADER_EVENT_TRACE` from ntwmi.h
    static constexpr inline std::uint32_t trace_message_flag            = 0x10; // shifted `TRACE_MESSAGE` from ntwmi.h

    inline bool is_trace_header() const { return (header_flags() & trace_header_flag) != 0; }
    inline bool is_trace_header_event_trace() const { return (header_flags() & trace_header_event_trace_flag) != 0; }
    inline bool is_trace_message() const { return (header_flags() & trace_message_flag) != 0; }
};

// See _WMI_TRACE_PACKET from ntwmi.h
struct wmi_trace_packet_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto size() const { return extract<std::uint16_t>(0); }
    inline auto type() const { return extract<std::uint8_t>(2); }
    inline auto group() const { return extract<event_trace_group>(3); }

    static inline constexpr std::size_t static_size = 4;
};

} // namespace snail::etl::parser
