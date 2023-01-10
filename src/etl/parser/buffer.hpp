
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

namespace perfreader::etl::parser {

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
struct etw_buffer_context
{
    union
    {
        struct
        {
            std::uint8_t processor_number;
            std::uint8_t alignment;
        } unnamed;
        std::uint16_t processor_index;
    };
    std::uint16_t logger_id;
};

std::size_t parse(utility::binary_stream_parser& stream, etw_buffer_context& data);

// See WNODE_HEADER from wmistr.h or the beginning of _WMI_BUFFER_HEADER from ntwmi.h
// The documentated version (from wmistr.h) differs slightly and does not specifify the
// ETW sub-structures
struct wnode_header
{
    std::uint32_t buffer_size;
    std::uint32_t saved_offset;
    std::uint32_t current_offset;
    std::int32_t reference_count;
    std::int64_t timestamp;
    std::int64_t sequence_number;
    union
    {
        struct
        {
            std::uint64_t type : 3;
            std::uint64_t frequency : 61;
        } unnamed;
        std::uint64_t clock;
    };
    etw_buffer_context client_context;
    etw_buffer_state state;
};

std::size_t parse(utility::binary_stream_parser& stream, wnode_header& data);

// See _WMI_BUFFER_HEADER from ntwmi.h
// There it has the WNODE_HEADER structure included inline
struct wmi_buffer_header
{
    wnode_header wnode;
    std::uint32_t offset;
    std::underlying_type_t<etw_buffer_flag> buffer_flag;
    etw_buffer_type buffer_type;
    struct
    {
        std::int64_t start_time;
        std::int64_t start_perf_clock;
    } reference_time;
};

std::size_t parse(utility::binary_stream_parser& stream, wmi_buffer_header& data);

} // namespace perfreader::etl::parser
