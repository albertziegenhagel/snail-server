
#pragma once

#include <snail/common/bit_flags.hpp>

namespace snail::perf_data::parser {

enum class header_feature
{
    tracing_data = 1,
    build_id,
    hostname,
    osrelease,
    version,
    arch,
    nr_cpus,
    cpu_desc,
    cpu_id,
    total_mem,
    cmdline,
    event_desc,
    cpu_topology,
    numa_topology,
    branch_stack,
    pmu_mappings,
    group_desc,
    aux_trace,
    stat,
    cache,
    sample_time,
    mem_topology,
    clockid,
    dir_format,
    bpf_prog_info,
    bpf_btf,
    compressed,
    cpu_pmu_caps,
    clock_data,
    hybrid_topology,
    pmu_caps,

    last_feature
};

using header_feature_flags = common::bit_flags<parser::header_feature, 256>;

} // namespace snail::perf_data::parser
