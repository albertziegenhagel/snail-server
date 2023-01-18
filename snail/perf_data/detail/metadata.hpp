
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data::detail {

struct event_attributes_database;

struct perf_data_metadata
{
    // build_id_event build_id;
    std::optional<std::string> hostname;
    std::optional<std::string> os_release;
    std::optional<std::string> version;
    std::optional<std::string> arch;
    struct nr_cpus_data
    {
        std::uint32_t nr_cpus_available;
        std::uint32_t nr_cpus_online;
    };
    std::optional<nr_cpus_data>             nr_cpus;
    std::optional<std::string>              cpu_desc;
    std::optional<std::string>              cpu_id;
    std::optional<std::uint64_t>            total_mem;
    std::optional<std::vector<std::string>> cmdline;
    struct event_desc_data
    {
        parser::event_attributes   attribute;
        std::string                event_string;
        std::vector<std::uint64_t> ids;
    };
    std::vector<event_desc_data> event_desc;
    // cpu_topology;
    // numa_topology;
    // branch_stack;
    // pmu_mappings;
    // group_desc;
    // aux_trace;
    // stat;
    // cache;
    struct sample_time_data
    {
        std::uint64_t start;
        std::uint64_t end;
    };
    std::optional<sample_time_data> sample_time;
    // mem_topology;
    std::optional<std::uint64_t>    clockid;
    // dir_format;
    // bpf_prog_info;
    // bpf_btf;
    // compressed;
    // cpu_pmu_caps;
    // clock_data;
    // hybrid_topology;
    // pmu_caps;

    void extract_event_attributes_database(event_attributes_database& database);
};

} // namespace snail::perf_data::detail
