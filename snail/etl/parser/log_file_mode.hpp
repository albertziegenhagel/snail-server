
#pragma once

#include <snail/common/bit_flags.hpp>

namespace snail::etl::parser {

enum class log_file_mode : std::uint32_t
{
    // file_mode_none             = 0,  // 0x0000'0000
    file_mode_sequential       = 0,  // 0x0000'0001
    file_mode_circular         = 1,  // 0x0000'0002
    file_mode_append           = 2,  // 0x0000'0004
    file_mode_newfile          = 3,  // 0x0000'0008
    use_ms_flush_timer         = 4,  // 0x0000'0010
    file_mode_preallocate      = 5,  // 0x0000'0020
    nonstoppable_mode          = 6,  // 0x0000'0040
    secure_mode                = 7,  // 0x0000'0080
    real_time_mode             = 8,  // 0x0000'0100
    delay_open_file_mode       = 9,  // 0x0000'0200
    buffering_mode             = 10, // 0x0000'0400
    private_logger_mode        = 11, // 0x0000'0800
    add_header_mode            = 12, // 0x0000'1000
    use_kbytes_for_size        = 13, // 0x0000'2000
    use_global_sequence        = 14, // 0x0000'4000
    use_local_sequence         = 15, // 0x0000'8000
    relog_mode                 = 16, // 0x0001'0000
    private_in_proc            = 17, // 0x0002'0000
    buffer_interface_mode      = 18, // 0x0004'0000
    kd_filter_mode             = 19, // 0x0008'0000
    realtime_relog_mode        = 20, // 0x0010'0000
    lost_events_debug_mode     = 21, // 0x0020'0000
    stop_on_hybrid_shutdown    = 22, // 0x0040'0000
    persist_on_hybrid_shutdown = 23, // 0x0080'0000
    use_paged_memory           = 24, // 0x0100'0000
    system_logger_mode         = 25, // 0x0200'0000
    compressed_mode            = 26, // 0x0400'0000
    independent_session_mode   = 27, // 0x0800'0000
    no_per_processor_buffering = 28, // 0x1000'0000
    event_trace_blocking_mode  = 29, // 0x2000'0000
    addto_triage_dump          = 31, // 0x8000'0000
};

using log_file_mode_flags = common::bit_flags<log_file_mode, 32>;

} // namespace snail::etl::parser
