
#pragma once

#include <cstdint>

#include <bit>

#include <snail/perf_data/parser/header_feature.hpp>

namespace snail::perf_data::detail {

struct perf_data_file_header_data
{
    struct section_data
    {
        std::uint64_t offset;
        std::uint64_t size;
    };

    std::endian   byte_order;
    std::uint64_t size;
    std::uint64_t attribute_size;

    section_data attributes;
    section_data data;
    section_data event_types;

    parser::header_feature_flags additional_features;
};

} // namespace snail::perf_data::detail
