
#pragma once

#include <cstdint>

#include <array>

#include "etl/parser/extract.hpp"
#include "etl/parser/utility.hpp"

//
// event records for event_trace_group::stackwalk
//

namespace perfreader::etl::parser {

// or _STACK_WALK_EVENT_DATA in ntwmi.h
// `StackWalk_Event:StackWalk` from wmicore.mof in WDK 10.0.22621.0
struct stackwalk_v2_stack_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        stack = 32
    };
    static inline constexpr std::array<std::uint8_t, 1>  event_types    = {32};
    static inline constexpr std::array<std::uint16_t, 1> event_versions = {2};

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto event_timestamp() const { return extract<std::uint64_t>(0); }
    inline auto process_id() const { return extract<std::uint32_t>(8); }
    inline auto thread_id() const { return extract<std::uint32_t>(12); }

    static constexpr inline auto stack_base_offset = 16;

    inline std::size_t stack_size() const
    {
        return (buffer_.size() - stack_base_offset) / pointer_size_;
    }
    inline std::uint64_t stack_address(std::size_t index) const
    {
        assert(index < stack_size());
        return extract_pointer(dynamic_offset(stack_base_offset, index)); 
    }

    inline std::size_t event_size() const
    {
        return stack_base_offset + stack_size() * pointer_size_;
    }
};

} // namespace perfreader::etl::parser
