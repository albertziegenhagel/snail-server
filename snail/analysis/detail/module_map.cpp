
#include <snail/analysis/detail/module_map.hpp>

#include <algorithm>
#include <cassert>
#include <ranges>

using namespace snail;
using namespace snail::analysis::detail;

struct module_map::address_range
{
    // half-open address range: [begin, end)
    std::uint64_t begin_address;
    std::uint64_t end_address;

    struct module_entry
    {
        data::timestamp_t load_timestamp;
        std::size_t       module_index;
    };

    // Sorted by load_timestamp (from oldest to newest)
    std::vector<module_entry> active_modules;

    [[nodiscard]] bool contains(data::instruction_pointer_t address) const noexcept
    {
        return address >= begin_address &&
               address < end_address;
    }

    void add_active_module(module_entry new_module)
    {
        // It is common that the latest module will be added last, hence we do not need to sort
        const auto simple_append = new_module.load_timestamp >= active_modules.back().load_timestamp;

        active_modules.push_back(std::move(new_module));

        if(!simple_append)
        {
            std::ranges::sort(active_modules, std::less<>(), &module_entry::load_timestamp);
        }
    }
};

module_map::module_map() = default;

module_map::~module_map() = default;

std::vector<module_info>& module_map::all_modules()
{
    return modules;
}

const std::vector<module_info>& module_map::all_modules() const
{
    return modules;
}

void module_map::insert(module_info module, data::timestamp_t load_timestamp)
{
    const auto new_module_index = modules.size();
    modules.push_back(std::move(module));

    auto to_insert_begin = modules.back().base;
    auto to_insert_end   = modules.back().base + modules.back().size;

    // If we do not have any ranges inserted yet, we can abort early.
    // This simplifies the remainder of this method
    if(address_ranges.empty())
    {
        auto new_range = address_range{
            .begin_address  = to_insert_begin,
            .end_address    = to_insert_end,
            .active_modules = {
                {load_timestamp, new_module_index}}};
        address_ranges.push_back(std::move(new_range));
        return;
    }

    // Check whether we need to enlarge the total address range at the beginning
    // and prepend a new range part if necessary.
    bool inserted_before = false;
    if(address_ranges.front().begin_address > to_insert_begin)
    {
        const auto current_total_begin = address_ranges.front().begin_address;

        auto new_range = address_range{
            .begin_address  = to_insert_begin,
            .end_address    = std::min(current_total_begin, to_insert_end),
            .active_modules = {
                {load_timestamp, new_module_index}}};
        address_ranges.insert(address_ranges.begin(), std::move(new_range));
        inserted_before = true;

        // Adjust the interval to insert to not contain the already added part
        to_insert_begin = current_total_begin;
        if(to_insert_begin >= to_insert_end) return;
    }

    // Check whether we need to enlarge the total address range at the end
    // and append a new range part if necessary.
    bool inserted_after = false;
    if(address_ranges.back().end_address < to_insert_end)
    {
        const auto current_total_end = address_ranges.back().end_address;

        auto new_range = address_range{
            .begin_address  = std::max(current_total_end, to_insert_begin),
            .end_address    = to_insert_end,
            .active_modules = {
                {load_timestamp, new_module_index}}};
        address_ranges.push_back(std::move(new_range));
        inserted_after = true;

        // Adjust the interval to insert to not contain the already added part
        to_insert_end = current_total_end;
        if(to_insert_end <= to_insert_begin) return;
    }

    // From this point on, the range to insert is a subset of the total address range
    assert(to_insert_begin >= address_ranges.front().begin_address &&
           to_insert_end <= address_ranges.back().end_address);

    auto first_range_after      = std::ranges::lower_bound(address_ranges, to_insert_end, std::less<>(), &address_range::begin_address);
    auto last_overlapping_range = std::prev(first_range_after);

    // Check if the new range to insert is completely between two existing ones
    // and can simply be added without any overlaps.
    // (in this case `last_overlapping_range` does not actually overlap)
    if(last_overlapping_range->end_address <= to_insert_begin)
    {
        auto new_range = address_range{
            .begin_address  = to_insert_begin,
            .end_address    = to_insert_end,
            .active_modules = {
                {load_timestamp, new_module_index}}};
        address_ranges.insert(first_range_after, std::move(new_range));
        return;
    }

    // If we reach this point, there are overlaps that we have to resolve

    // Find the first range that overlaps the interval to be added.
    // We short optimize the call to `lower_bound` away in the case that there is only
    // a single overlapping range (this can be quite common when the existing range and
    // the range to be inserted to exactly overlap)
    auto first_overlapping_range = last_overlapping_range->begin_address <= to_insert_begin ?
                                       last_overlapping_range :
                                       std::ranges::lower_bound(std::views::reverse(address_ranges), to_insert_begin, std::greater<>(), &address_range::end_address).base();

    // Handle a special case: The module we insert does exactly match an existing one
    if(first_overlapping_range == last_overlapping_range &&
       first_overlapping_range->begin_address == modules.back().base &&
       first_overlapping_range->end_address == (modules.back().base + modules.back().size) &&
       first_overlapping_range->active_modules.back().load_timestamp < load_timestamp &&
       modules[first_overlapping_range->active_modules.back().module_index].file_name == modules.back().file_name)
    {
        assert(!inserted_before && !inserted_after);
        const auto& last_active_module = modules[first_overlapping_range->active_modules.back().module_index];
        assert(last_active_module.base == modules.back().base);
        assert(last_active_module.size == modules.back().size);
        modules.pop_back();
        return;
    }

    assert(first_overlapping_range <= last_overlapping_range);

    // Check whether we need to split the range that overlaps at the beginning
    if(first_overlapping_range->begin_address < to_insert_begin)
    {
        auto new_range = address_range{
            .begin_address  = to_insert_begin,
            .end_address    = first_overlapping_range->end_address,
            .active_modules = first_overlapping_range->active_modules};

        first_overlapping_range->end_address = new_range.begin_address;

        // Insert the new range and make sure both `first_overlapping_range` and
        // `last_overlapping_range` are still valid iterators
        const auto last_index   = last_overlapping_range - address_ranges.begin();
        first_overlapping_range = address_ranges.insert(std::next(first_overlapping_range), std::move(new_range));
        last_overlapping_range  = address_ranges.begin() + last_index + 1;
    }

    // Check whether we need to split the range that overlaps at the end
    if(last_overlapping_range->end_address > to_insert_end)
    {
        auto new_range = address_range{
            .begin_address  = last_overlapping_range->begin_address,
            .end_address    = to_insert_end,
            .active_modules = last_overlapping_range->active_modules};

        last_overlapping_range->begin_address = new_range.end_address;

        // Insert the new range and make sure both `first_overlapping_range` and
        // `last_overlapping_range` are still valid iterators
        const auto first_index  = first_overlapping_range - address_ranges.begin();
        last_overlapping_range  = address_ranges.insert(last_overlapping_range, std::move(new_range));
        first_overlapping_range = address_ranges.begin() + first_index;
    }

    // Add the new module to all overlapping ranges
    for(auto iter = first_overlapping_range, end = std::next(last_overlapping_range); iter != end; ++iter)
    {
        iter->add_active_module({load_timestamp, new_module_index});
    }
}

std::pair<const module_info*, data::timestamp_t> module_map::find(data::instruction_pointer_t address, data::timestamp_t timestamp, bool strict) const
{
    auto reversed_ranges = std::views::reverse(address_ranges);

    const auto iter = std::ranges::lower_bound(reversed_ranges, address, std::greater<>(), &address_range::begin_address);
    if(iter == reversed_ranges.end()) return {nullptr, 0};
    if(!iter->contains(address)) return {nullptr, 0};

    assert(!iter->active_modules.empty());

    auto latest_module_iter = std::prev(iter->active_modules.end());
    while(latest_module_iter->load_timestamp > timestamp && latest_module_iter != iter->active_modules.begin())
    {
        --latest_module_iter;
    }

    if(strict && latest_module_iter->load_timestamp > timestamp) return {nullptr, 0};

    return {&modules[latest_module_iter->module_index], latest_module_iter->load_timestamp};
}
