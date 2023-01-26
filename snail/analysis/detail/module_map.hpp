#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <snail/data/types.hpp>

namespace snail::analysis::detail {

struct module_info
{
    std::uint64_t base;
    std::uint64_t size;

    std::string   file_name;
    std::uint64_t page_offset;
};

class module_map
{
public:
    module_map();
    ~module_map();

    std::vector<module_info>&       all_modules();
    const std::vector<module_info>& all_modules() const;

    void insert(module_info module, data::timestamp_t load_timestamp);

    // FIXME: improve/remove returning load timestamp
    std::pair<const module_info*, data::timestamp_t> find(data::instruction_pointer_t address, data::timestamp_t timestamp, bool strict = true) const;

private:
    struct address_range;
    struct address_range_begin_less;

    std::vector<module_info> modules;

    std::vector<address_range> address_ranges;
};

} // namespace snail::analysis::detail
