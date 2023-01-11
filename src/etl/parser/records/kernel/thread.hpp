
#pragma once

#include <cstdint>

#include <array>
#include <string>
#include <optional>

#include "etl/parser/extract.hpp"
#include "etl/parser/utility.hpp"

//
// event records for event_trace_group::thread
//

namespace perfreader::etl::parser {

// `Thread_V3_TypeGroup1:Thread_V3` from wmicore.mof in WDK 10.0.22621.0
struct thread_v3_type_group1_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        start    = 1,
        end      = 2,
        dc_start = 3,
        dc_end   = 4,
    };
    static inline constexpr std::array<std::uint8_t, 4>  event_types    = {1, 2, 3, 4};
    static inline constexpr std::array<std::uint16_t, 2> event_versions = {3};

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
};

// `Thread_TypeGroup1:Thread_V4` from wmicore.mof in WDK 10.0.22621.0
struct thread_v4_type_group1_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        start    = 1,
        end      = 2,
        dc_start = 3,
        dc_end   = 4,
    };
    static inline constexpr std::array<std::uint8_t, 4>  event_types    = {1, 2, 3, 4};
    static inline constexpr std::array<std::uint16_t, 2> event_versions = {4};

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
private:
    mutable std::optional<std::size_t> thread_name_length;
};

} // namespace perfreader::etl::parser
