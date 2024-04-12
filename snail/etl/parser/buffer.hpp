
#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/etl/parser/extract.hpp>

namespace snail::etl::parser {

// See _ETW_BUFFER_STATE from ntwmi.h
enum class etw_buffer_state : std::uint32_t
{
    free_               = 0,
    general_logging     = 2,
    flush               = 3,
    pending_compression = 4,
    compressed          = 5,
    placeholder         = 6,
    maximum             = 7
};

// See ETW_BUFFER_FLAG_* from ntwmi.h
enum class etw_buffer_flag : std::uint16_t
{
    normal           = 0,
    flush_marker     = 1,
    events_lost      = 2,
    buffer_lost      = 4,
    rtbackup_corrupt = 8,
    rtbackup         = 16,
    proc_index       = 32,
    compressed       = 64
};

// See ETW_BUFFER_TYPE_* from ntwmi.h
enum class etw_buffer_type : std::uint16_t
{
    generic      = 0,
    rundown      = 1,
    ctx_swap     = 2,
    reftime      = 3,
    header       = 4,
    batched      = 5,
    empty_marker = 6,
    dbg_info     = 7,
    maximum      = 8
};

// See ETW_BUFFER_CONTEXT from evntrace.h
struct etw_buffer_context_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto processor_index() const { return extract<std::uint16_t>(0); }
    inline auto logger_id() const { return extract<std::uint16_t>(2); }

    static inline constexpr std::size_t static_size = 4;
};

// See WNODE_HEADER from wmistr.h or the beginning of _WMI_BUFFER_HEADER from ntwmi.h
// The documented version (from wmistr.h) differs slightly and does not specify the
// ETW sub-structures
struct wnode_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto buffer_size() const { return extract<std::uint32_t>(0); }
    inline auto saved_offset() const { return extract<std::uint32_t>(4); }
    inline auto current_offset() const { return extract<std::uint32_t>(8); }
    inline auto reference_count() const { return extract<std::int32_t>(12); }
    inline auto timestamp() const { return extract<std::int64_t>(16); }
    inline auto sequence_number() const { return extract<std::int64_t>(24); }
    inline auto clock() const { return extract<std::uint64_t>(32); }
    inline auto client_context() const { return etw_buffer_context_view(buffer().subspan(40)); }
    inline auto state() const { return extract<etw_buffer_state>(40 + etw_buffer_context_view::static_size); }

    static inline constexpr std::size_t static_size = 44 + etw_buffer_context_view::static_size;
};

// See _WMI_BUFFER_HEADER from ntwmi.h
// There it has the WNODE_HEADER structure included inline

struct wmi_buffer_header_view : private extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto wnode() const { return wnode_header_view(buffer()); }
    inline auto offset() const { return extract<std::uint32_t>(0 + wnode_header_view::static_size); }
    inline auto buffer_flag() const { return extract<std::underlying_type_t<etw_buffer_flag>>(4 + wnode_header_view::static_size); }
    inline auto buffer_type() const { return extract<etw_buffer_type>(6 + wnode_header_view::static_size); }
    inline auto start_time() const { return extract<std::uint64_t>(8 + wnode_header_view::static_size); }
    inline auto start_perf_clock() const { return extract<std::uint64_t>(16 + wnode_header_view::static_size); }

    static inline constexpr std::size_t static_size = 24 + wnode_header_view::static_size;
};

} // namespace snail::etl::parser
