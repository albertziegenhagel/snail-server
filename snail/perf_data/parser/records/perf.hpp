#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/perf_data/parser/event.hpp>

namespace snail::perf_data::parser {

struct finished_round_event_view : private parser::event_view_base
{
    static inline constexpr parser::event_type event_type = parser::event_type::finished_round;

    using event_view_base::event_view_base;
};

struct finished_init_event_view : private parser::event_view_base
{
    static inline constexpr parser::event_type event_type = parser::event_type::finished_init;

    using event_view_base::event_view_base;
};

struct id_index_entry_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto id() const { return extract<std::uint64_t>(0); }
    inline auto idx() const { return extract<std::uint64_t>(8); }
    inline auto cpu() const { return extract<std::uint64_t>(16); }
    inline auto tid() const { return extract<std::uint64_t>(24); }

    static inline constexpr std::size_t static_size = 32;
};

struct id_index_event_view : private parser::event_view_base
{
    static inline constexpr parser::event_type event_type = parser::event_type::id_index;

    using event_view_base::event_view_base;

    inline auto nr() const { return extract<std::uint64_t>(0); }
    inline auto entry(std::size_t index) const { return id_index_entry_view(buffer().subspan(8 + index * id_index_entry_view::static_size), byte_order()); }
};

struct thread_map_entry_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto pid() const { return extract<std::uint64_t>(0); }
    inline auto comm() const { return std::string_view(reinterpret_cast<const char*>(buffer().data() + 8), 16); }

    static inline constexpr std::size_t static_size = 24;
};

struct thread_map_event_view : private parser::event_view_base
{
    static inline constexpr parser::event_type event_type = parser::event_type::thread_map;

    using event_view_base::event_view_base;

    inline auto nr() const { return extract<std::uint64_t>(0); }
    inline auto entry(std::size_t index) const { return thread_map_entry_view(buffer().subspan(8 + index * thread_map_entry_view::static_size), byte_order()); }
};

enum class cpu_map_type : std::uint16_t
{
    cpus       = 0,
    mask       = 1,
    range_cpus = 2,
};

struct cpu_map_entries_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto nr() const { return extract<std::uint16_t>(0); }
    inline auto cpu(std::size_t index) const
    {
        assert(index < nr());
        return extract<std::uint16_t>(2 + index * 2);
    }
};

struct mask_cpu_map_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto nr() const { return extract<std::uint16_t>(0); }
    inline auto long_size() const { return extract<std::uint16_t>(2); }
    inline auto mask_32(std::size_t index) const
    {
        assert(long_size() == 4);
        assert(index < nr());
        return extract<std::uint16_t>(4 + index * 4);
    }
    inline auto mask_64(std::size_t index) const
    {
        assert(long_size() == 8);
        assert(index < nr());
        return extract<std::uint16_t>(8 + index * 8);
    }
};

struct range_cpu_map_view : private common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto any_cpu() const { return extract<std::uint8_t>(0); }
    inline auto start_cpu() const { return extract<std::uint16_t>(2); }
    inline auto end_cpu() const { return extract<std::uint16_t>(4); }
};

struct cpu_map_event_view : private parser::event_view_base
{
    static inline constexpr parser::event_type event_type = parser::event_type::cpu_map;

    using event_view_base::event_view_base;

    inline auto type() const { return extract<cpu_map_type>(0); }

    inline auto cpus_data() const
    {
        assert(type() == cpu_map_type::cpus);
        return cpu_map_entries_view(buffer().subspan(2), byte_order());
    }
    inline auto mask_data() const
    {
        assert(type() == cpu_map_type::mask);
        return mask_cpu_map_view(buffer().subspan(2), byte_order());
    }
    inline auto range_cpus_data() const
    {
        assert(type() == cpu_map_type::range_cpus);
        return range_cpu_map_view(buffer().subspan(2), byte_order());
    }
};

} // namespace snail::perf_data::parser
