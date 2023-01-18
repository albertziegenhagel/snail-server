
#pragma once

#include <cstdint>

#include <array>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// event records for event_trace_group::stackwalk
//

namespace snail::etl::parser {

// or _STACK_WALK_EVENT_DATA in ntwmi.h
// `StackWalk_Event:StackWalk` from wmicore.mof in WDK 10.0.22621.0
struct stackwalk_v2_stack_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::stackwalk, 32, "stack"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto event_timestamp() const { return extract<std::uint64_t>(0); }
    inline auto process_id() const { return extract<std::uint32_t>(8); }
    inline auto thread_id() const { return extract<std::uint32_t>(12); }

    static constexpr inline auto stack_base_offset = 16;

    inline std::size_t stack_size() const
    {
        return (buffer().size() - stack_base_offset) / pointer_size();
    }
    inline std::uint64_t stack_address(std::size_t index) const
    {
        assert(index < stack_size());
        return extract_pointer(dynamic_offset(stack_base_offset, index));
    }

    inline std::size_t event_size() const
    {
        return stack_base_offset + stack_size() * pointer_size();
    }
};

} // namespace snail::etl::parser
