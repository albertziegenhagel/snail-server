
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// event records for event_trace_group::thread
//

namespace snail::etl::parser {

// `ThreadSetName:Thread_V2` from wmicore.mof in WDK 10.0.22621.0
struct thread_v2_set_name_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Thread-SetName";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::thread, 72, "SetName"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }

    inline auto thread_name() const { return extract_u16string(dynamic_offset(8, 0), thread_name_length); }

    inline std::size_t dynamic_size() const { return dynamic_offset(8 + thread_name().size() * 2 + 2, 0); }

private:
    mutable std::optional<std::size_t> thread_name_length;
};

// `Thread_V3_TypeGroup1:Thread_V3` from wmicore.mof in WDK 10.0.22621.0
struct thread_v3_type_group1_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Thread-TypeGroup1";
    static inline constexpr std::uint16_t    event_version = 3;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::thread, 1, "Start"  },
        event_identifier_group{event_trace_group::thread, 2, "End"    },
        event_identifier_group{event_trace_group::thread, 3, "DcStart"},
        event_identifier_group{event_trace_group::thread, 4, "DcEnd"  }
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }

    inline auto stack_base() const { return extract_pointer(dynamic_offset(8, 0)); }
    inline auto stack_limit() const { return extract_pointer(dynamic_offset(8, 1)); }

    inline auto user_stack_base() const { return extract_pointer(dynamic_offset(8, 2)); }
    inline auto user_stack_limit() const { return extract_pointer(dynamic_offset(8, 3)); }

    inline auto affinity() const { return extract_pointer(dynamic_offset(8, 4)); }

    inline auto win32_start_addr() const { return extract_pointer(dynamic_offset(8, 5)); }
    inline auto teb_base() const { return extract_pointer(dynamic_offset(8, 6)); }

    inline auto sub_process_tag() const { return extract<std::uint32_t>(dynamic_offset(8, 7)); }

    inline auto base_priority() const { return extract<std::uint8_t>(dynamic_offset(12, 7)); }
    inline auto page_priority() const { return extract<std::uint8_t>(dynamic_offset(13, 7)); }
    inline auto io_priority() const { return extract<std::uint8_t>(dynamic_offset(14, 7)); }

    inline auto flags() const { return extract<std::uint8_t>(dynamic_offset(15, 7)); }

    inline auto thread_name() const
    {
        if(buffer().size() <= dynamic_offset(16, 7)) return std::optional<std::u16string_view>{};
        return std::make_optional(extract_u16string(dynamic_offset(16, 7), thread_name_length));
    }

    inline std::size_t dynamic_size() const { return dynamic_offset(16, 7) + (thread_name() ? (thread_name()->size() * 2 + 2) : 0); }

private:
    mutable std::optional<std::size_t> thread_name_length;
};

// `Thread_TypeGroup1:Thread_V4` from wmicore.mof in WDK 10.0.22621.0
struct thread_v4_type_group1_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Thread-TypeGroup1";
    static inline constexpr std::uint16_t    event_version = 4;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::thread, 1, "Start"  },
        event_identifier_group{event_trace_group::thread, 2, "End"    },
        event_identifier_group{event_trace_group::thread, 3, "DcStart"},
        event_identifier_group{event_trace_group::thread, 4, "DcEnd"  }
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto thread_id() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }

    inline auto stack_base() const { return extract_pointer(dynamic_offset(8, 0)); }
    inline auto stack_limit() const { return extract_pointer(dynamic_offset(8, 1)); }

    inline auto user_stack_base() const { return extract_pointer(dynamic_offset(8, 2)); }
    inline auto user_stack_limit() const { return extract_pointer(dynamic_offset(8, 3)); }

    inline auto affinity() const { return extract_pointer(dynamic_offset(8, 4)); }

    inline auto win32_start_addr() const { return extract_pointer(dynamic_offset(8, 5)); }
    inline auto teb_base() const { return extract_pointer(dynamic_offset(8, 6)); }

    inline auto sub_process_tag() const { return extract<std::uint32_t>(dynamic_offset(8, 7)); }

    inline auto base_priority() const { return extract<std::uint8_t>(dynamic_offset(12, 7)); }
    inline auto page_priority() const { return extract<std::uint8_t>(dynamic_offset(13, 7)); }
    inline auto io_priority() const { return extract<std::uint8_t>(dynamic_offset(14, 7)); }

    inline auto flags() const { return extract<std::uint8_t>(dynamic_offset(15, 7)); }

    inline auto thread_name() const { return extract_u16string(dynamic_offset(16, 7), thread_name_length); }

    inline std::size_t dynamic_size() const { return dynamic_offset(16 + thread_name().size() * 2 + 2, 7); }

private:
    mutable std::optional<std::size_t> thread_name_length;
};

// `CSwitch_V4:Thread_V4` from wmicore.mof in WDK 10.0.22621.0
struct thread_v4_context_switch_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "Thread-ContextSwitch";
    static inline constexpr std::uint16_t    event_version = 4;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::thread, 36, "ContextSwitch"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto new_thread_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto old_thread_id() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }

    inline auto new_thread_priority() const { return extract<std::int8_t>(dynamic_offset(8, 0)); }
    inline auto old_thread_priority() const { return extract<std::int8_t>(dynamic_offset(9, 0)); }

    inline auto previous_c_state() const { return extract<std::uint8_t>(dynamic_offset(10, 0)); }

    inline auto spare_byte() const { return extract<std::int8_t>(dynamic_offset(11, 0)); }

    inline auto old_thread_wait_reason() const { return extract<std::int8_t>(dynamic_offset(12, 0)); }

    inline auto thread_flags() const { return extract<std::int8_t>(dynamic_offset(13, 0)); }

    inline auto old_thread_state() const { return extract<std::int8_t>(dynamic_offset(14, 0)); }

    inline auto old_thread_wait_ideal_processor() const { return extract<std::int8_t>(dynamic_offset(15, 0)); }

    inline auto new_thread_wait_time() const { return extract<std::uint32_t>(dynamic_offset(16, 0)); }

    inline auto reserved() const { return extract<std::uint32_t>(dynamic_offset(20, 0)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(24, 0); }
};

} // namespace snail::etl::parser
