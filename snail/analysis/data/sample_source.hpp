#pragma once

#include <string_view>

namespace snail::analysis {

struct sample_source_info
{
    using id_t = std::size_t;

    id_t id;

    std::string name;

    std::size_t number_of_samples;
    double      average_sampling_rate; // in samples per second

    bool has_stacks;
};

} // namespace snail::analysis
