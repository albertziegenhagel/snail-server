#pragma once

#include <cstdint>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <snail/common/bit_flags.hpp>
#include <snail/common/parser/extract.hpp>

namespace snail::perf_data::parser {

enum attribute_type : std::uint32_t
{
    hardware   = 0,
    software   = 1,
    tracepoint = 2,
    hw_cache   = 3,
    raw        = 4,
    breakpoint = 5
};

enum class sample_format
{
    ip             = 0,
    tid            = 1,
    time           = 2,
    addr           = 3,
    read           = 4,
    call_chain     = 5,
    id             = 6,
    cpu            = 7,
    period         = 8,
    stream_id      = 9,
    raw            = 10,
    branch_stack   = 11,
    regs_user      = 12,
    stack_user     = 13,
    weight         = 14,
    data_src       = 15,
    identifier     = 16,
    transaction    = 17,
    regs_intr      = 18,
    phys_addr      = 19,
    aux            = 20,
    cgroup         = 21,
    data_page_size = 22,
    code_page_size = 23,
    weight_struct  = 24
};

using sample_format_flags = common::bit_flags<sample_format, 64>;

enum class read_format
{
    total_time_enabled = 0,
    total_time_running = 1,
    id                 = 2,
    group              = 3,
    lost               = 4
};

using read_format_flags = common::bit_flags<read_format, 64>;

enum class attribute_flag
{
    disabled                 = 0,  // off by default
    inherit                  = 1,  // children inherit it
    pinned                   = 2,  // must always be on PMU
    exclusive                = 3,  // only group on PMU
    exclude_user             = 4,  // don't count user
    exclude_kernel           = 5,  // ditto kernel
    exclude_hv               = 6,  // ditto hypervisor
    exclude_idle             = 7,  // don't count when idle
    mmap                     = 8,  // include mmap data
    comm                     = 9,  // include comm data
    freq                     = 10, // use freq, not period
    inherit_stat             = 11, // per task counts
    enable_on_exec           = 12, // next exec enables
    task                     = 13, // trace fork/exit
    watermark                = 14, // wakeup_watermark
    // precise_ip               =  15 & 16,//skid constraint
    mmap_data                = 17, // non-exec mmap data
    sample_id_all            = 18, // sample_type all events
    exclude_host             = 19, // don't count in host
    exclude_guest            = 20, // don't count in guest
    exclude_callchain_kernel = 21, // exclude kernel callchains
    exclude_callchain_user   = 22, // exclude user callchains
    mmap2                    = 23, // include mmap with inode data
    comm_exec                = 24, // flag comm events that are due to an exec
    use_clockid              = 25, // use @clockid for time fields
    context_switch           = 26, // context switch data
    write_backward           = 27, // Write ring buffer from end to beginning
    namespaces               = 28, // include namespaces data
    ksymbol                  = 29, // include ksymbol events
    bpf_event                = 30, // include bpf events
    aux_output               = 31, // generate AUX records instead of events
    cgroup                   = 32, // include cgroup events
    text_poke                = 33, // include text poke events
    build_id                 = 34, // use build id in mmap2 events
    inherit_thread           = 35, // children only inherit if cloned with CLONE_THREAD
    remove_on_exec           = 36, // event is removed from task on exec
    sigtrap                  = 37  // send synchronous SIGTRAP on event
};

using attribute_flags = common::bit_flags<attribute_flag, 64>;

enum class skid_constraint_type
{
    can_have_arbitrary_skid,
    must_have_constant_skid,
    requested_to_have_0_skid,
    must_have_0_skid
};

struct event_attributes
{
    attribute_type type;
    // std::uint64_t config;
    std::uint64_t  sample_period_freq; // either the sample period or the sample frequency, depending on the `attribute_flag::freq` flag is set.

    sample_format_flags sample_format;
    read_format_flags   read_format;

    attribute_flags      flags;
    skid_constraint_type precise_ip;

    std::optional<std::string> name;
};

struct event_attributes_view : protected common::parser::extract_view_base
{
    using extract_view_base::extract_view_base;

    inline auto type() const { return extract<attribute_type>(0); }
    inline auto size() const { return extract<std::uint32_t>(4); }
    inline auto config() const { return extract<std::uint64_t>(8); }

    // union
    inline auto sample_period() const
    {
        assert(!flags().test(attribute_flag::freq));
        return extract<std::uint64_t>(16);
    }
    inline auto sample_freq() const
    {
        assert(flags().test(attribute_flag::freq));
        return extract<std::uint64_t>(16);
    }

    inline auto sample_format() const { return sample_format_flags(extract<std::uint64_t>(24)); }
    inline auto read_format() const { return read_format_flags(extract<std::uint64_t>(32)); }

    inline attribute_flags flags() const { return attribute_flags(extract<std::uint64_t>(40)); }
    inline auto            precise_ip() const
    {
        const auto precise_ip_type = (extract<std::uint64_t>(40) >> 15) & 0b11; // extract bits 15 & 16
        switch(precise_ip_type)
        {
        case 0: return skid_constraint_type::can_have_arbitrary_skid;
        case 1: return skid_constraint_type::must_have_constant_skid;
        case 2: return skid_constraint_type::requested_to_have_0_skid;
        case 3: return skid_constraint_type::must_have_0_skid;
        }
        std::unreachable();
    }

    // union
    inline auto wakeup_events() const
    {
        assert(!flags().test(attribute_flag::watermark));
        return extract<std::uint32_t>(48);
    }
    inline auto wakeup_watermark() const
    {
        assert(flags().test(attribute_flag::watermark));
        return extract<std::uint32_t>(48);
    }

    inline auto bp_type() const { return extract<std::uint32_t>(52); }

    // union
    inline auto bp_addr() const { return extract<std::uint64_t>(56); }
    inline auto kprobe_func() const { return extract<std::uint64_t>(56); }
    inline auto uprobe_path() const { return extract<std::uint64_t>(56); }
    inline auto config1() const { return extract<std::uint64_t>(56); }

    // union
    inline auto bp_len() const { return extract<std::uint64_t>(64); }
    inline auto kprobe_addr() const { return extract<std::uint64_t>(64); }  /* when kprobe_func == NULL */
    inline auto probe_offset() const { return extract<std::uint64_t>(64); } /* for perf_[k,u]probe */
    inline auto config2() const { return extract<std::uint64_t>(64); }

    inline auto branch_sample_type() const { return extract<std::uint64_t>(72); } // enum perf_branch_sample_type

    inline auto sample_regs_user() const { return extract<std::uint64_t>(80); }
    inline auto sample_stack_user() const { return extract<std::uint32_t>(88); }

    inline auto clockid() const { return extract<std::int32_t>(92); }

    inline auto sample_regs_intr() const { return extract<std::uint64_t>(96); }

    inline auto aux_watermark() const { return extract<std::uint32_t>(104); }
    inline auto sample_max_stack() const { return extract<std::uint16_t>(108); }
    // inline auto reserved_2() const { return extract<std::uint16_t>(110); }
    inline auto aux_sample_size() const { return extract<std::uint32_t>(112); }
    // inline auto reserved_3() const { return extract<std::uint32_t>(116); }

    inline auto sig_data() const { return extract<std::uint64_t>(120); }

    inline auto config3() const { return extract<std::uint64_t>(128); }

    static inline constexpr std::size_t static_size = 134;

    inline auto instantiate() const
    {
        return event_attributes{
            .type               = type(),
            .sample_period_freq = flags().test(attribute_flag::freq) ? sample_freq() : sample_period(),
            .sample_format      = sample_format(),
            .read_format        = read_format(),
            .flags              = flags(),
            .precise_ip         = precise_ip(),
            .name               = {},
        };
    }
};

} // namespace snail::perf_data::parser
