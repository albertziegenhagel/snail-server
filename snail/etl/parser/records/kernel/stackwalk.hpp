
#pragma once

#include <cstdint>

#include <array>
#include <ranges>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// event records for event_trace_group::stackwalk
//

namespace snail::etl::parser {

namespace detail {

template<typename EventView>
struct stack_iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::uint64_t;
    using reference         = std::uint64_t;
    using pointer           = std::uint64_t*;

    reference operator*() const
    {
        return event_view->stack_address(index);
    }

    stack_iterator& operator++()
    {
        ++index;
        return *this;
    }
    stack_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    stack_iterator& operator--()
    {
        --index;
        return *this;
    }
    stack_iterator operator--(int)
    {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    stack_iterator& operator+=(difference_type n)
    {
        index += n;
        return *this;
    }

    stack_iterator& operator-=(difference_type n)
    {
        index -= n;
        return *this;
    }

    friend constexpr stack_iterator operator+(const stack_iterator& i, difference_type n)
    {
        return {i.event_view, i.index + n};
    }
    friend constexpr stack_iterator operator-(const stack_iterator& i, difference_type n)
    {
        return {i.event_view, i.index - n};
    }

    friend bool operator==(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index == rhs.index;
    }
    friend bool operator!=(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index != rhs.index;
    }
    friend bool operator<(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index < rhs.index;
    }
    friend bool operator>(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index > rhs.index;
    }
    friend bool operator<=(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index <= rhs.index;
    }
    friend bool operator>=(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return lhs.index >= rhs.index;
    }

    friend difference_type operator-(const stack_iterator& lhs, const stack_iterator& rhs)
    {
        return difference_type(lhs.index) - difference_type(rhs.index);
    }

    const EventView* event_view;
    std::size_t      index;
};

} // namespace detail

// or _STACK_WALK_EVENT_DATA in ntwmi.h
// `StackWalk_Event:StackWalk` from wmicore.mof in WDK 10.0.22621.0
struct stackwalk_v2_stack_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "StackWalk-Stack";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::stackwalk, 32, "Stack"}
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

    inline std::size_t dynamic_size() const { return dynamic_offset(stack_base_offset, stack_size()); }

    inline std::ranges::subrange<detail::stack_iterator<stackwalk_v2_stack_event_view>> stack() const
    {
        return {
            detail::stack_iterator<stackwalk_v2_stack_event_view>{this, 0           },
            detail::stack_iterator<stackwalk_v2_stack_event_view>{this, stack_size()}
        };
    }
};

// `StackWalk_Key:StackWalk` from wmicore.mof in WDK 10.0.22621.0
struct stackwalk_v2_key_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "StackWalk-Key";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::stackwalk, 37, "Kernel"},
        event_identifier_group{event_trace_group::stackwalk, 38, "User"  }
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto event_timestamp() const { return extract<std::uint64_t>(0); }
    inline auto process_id() const { return extract<std::uint32_t>(8); }
    inline auto thread_id() const { return extract<std::uint32_t>(12); }
    inline auto stack_key() const { return extract_pointer(16); }

    inline std::size_t dynamic_size() const { return dynamic_offset(16, 1); }
};

// or _STACK_WALK_EVENT_DATA in ntwmi.h
// `StackWalk_TypeGroup1:StackWalk` from wmicore.mof in WDK 10.0.22621.0
struct stackwalk_v2_type_group1_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "StackWalk-TypeGroup1";
    static inline constexpr std::uint16_t    event_version = 2;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_group{event_trace_group::stackwalk, 34, "Create" },
        event_identifier_group{event_trace_group::stackwalk, 35, "Delete" },
        event_identifier_group{event_trace_group::stackwalk, 36, "Rundown"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto stack_key() const { return extract_pointer(0); }

    inline std::size_t stack_size() const
    {
        return (buffer().size() - pointer_size()) / pointer_size();
    }
    inline std::uint64_t stack_address(std::size_t index) const
    {
        assert(index < stack_size());
        return extract_pointer(dynamic_offset(0, 1 + index));
    }

    inline std::size_t dynamic_size() const { return dynamic_offset(0, 1 + stack_size()); }

    inline std::ranges::subrange<detail::stack_iterator<stackwalk_v2_type_group1_event_view>> stack() const
    {
        return {
            detail::stack_iterator<stackwalk_v2_type_group1_event_view>{this, 0           },
            detail::stack_iterator<stackwalk_v2_type_group1_event_view>{this, stack_size()}
        };
    }
};

} // namespace snail::etl::parser
