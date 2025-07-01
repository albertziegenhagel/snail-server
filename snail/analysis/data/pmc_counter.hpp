#pragma once

#include <optional>
#include <string>

namespace snail::analysis {

struct pmc_counter_info
{
    std::optional<std::string> name  = std::nullopt;
    std::size_t                count = 0;
};

} // namespace snail::analysis
