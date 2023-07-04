#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/common/parser/extract.hpp>

#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data::parser {

enum class event_type : std::uint32_t
{
    // Kernel events
    // https://github.com/torvalds/linux/blob/master/include/uapi/linux/perf_event.h
    // vvvvvv
    mmap             = 1,
    lost             = 2,
    comm             = 3,
    exit             = 4,
    throttle         = 5,
    unthrottle       = 6,
    fork             = 7,
    read             = 8,
    sample           = 9,
    mmap2            = 10,
    aux              = 11,
    itrace_start     = 12,
    lost_samples     = 13,
    switch_          = 14,
    switch_cpu_wide  = 15,
    namespaces       = 16,
    ksymbol          = 17,
    bpf_event        = 18,
    cgroup           = 19,
    text_poke        = 20,
    aux_output_hw_id = 21,
    // ^^^^^^
    // Kernel events

    // Perf events
    // https://github.com/torvalds/linux/blob/master/tools/lib/perf/include/perf/event.h
    // vvvvvv
    header_attr         = 64,
    header_event_type   = 65,
    header_tracing_data = 66,
    header_build_id     = 67,
    finished_round      = 68,
    id_index            = 69,
    auxtrace_info       = 70,
    auxtrace            = 71,
    auxtrace_error      = 72,
    thread_map          = 73,
    cpu_map             = 74,
    stat_config         = 75,
    stat                = 76,
    stat_round          = 77,
    event_update        = 78,
    time_conv           = 79,
    header_feature      = 80,
    compressed          = 81,
    finished_init       = 82
    // ^^^^^^
    // Perf events
};

enum class header_misc_mask : std::uint16_t
{
    // the first three bytes are a cpu mode mask
    cpumode_mask    = 0b111,
    cpumode_unknown = 0b000,
    kernel          = 0b001,
    user            = 0b010,
    hypervisor      = 0b011,
    guest_kernel    = 0b100,
    guest_user      = 0b101,

    proc_map_parse_timeout = (1 << 12),

    mmap_data  = (1 << 13), // on mmap events
    comm_exec  = (1 << 13), // on comm events
    fork_exec  = (1 << 13), // on fork events
    switch_out = (1 << 13), // on switch events

    exact_ip           = (1 << 14), // on sample events
    switch_out_preempt = (1 << 14), // on switch events
    mmap_build_id      = (1 << 14), // on mmap events

    ext_reserved = (1 << 15),
};

struct event_header_view : protected common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto type() const { return extract<event_type>(0); }
    inline auto misc() const { return extract<std::uint16_t>(4); }
    inline auto size() const { return extract<std::uint16_t>(6); }

    using extract_view_base::buffer;

    static inline constexpr std::size_t static_size = 8;
};

struct event_view_base : protected common::parser::extract_view_base
{
    inline explicit event_view_base(std::span<const std::byte> buffer, std::endian byte_order) :
        extract_view_base(buffer.subspan(event_header_view::static_size), byte_order),
        header_view_(buffer.subspan(0, event_header_view::static_size), byte_order)
    {}

    event_header_view header() const
    {
        return header_view_;
    }

    using extract_view_base::buffer;

private:
    event_header_view header_view_;
};

struct attribute_event_view_base : protected event_view_base
{
    inline explicit attribute_event_view_base(const event_attributes& attributes, std::span<const std::byte> buffer, std::endian byte_order) :
        event_view_base(buffer, byte_order),
        attributes_(&attributes)
    {}

    using event_view_base::buffer;
    using event_view_base::header;

protected:
    const event_attributes& attributes() const
    {
        return *attributes_;
    }

private:
    const event_attributes* attributes_;
};

template<typename T>
T parse_event(const event_attributes&    attributes,
              std::span<const std::byte> buffer,
              std::endian                byte_order);

} // namespace snail::perf_data::parser
