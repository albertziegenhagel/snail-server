
#pragma once

#include <bit>
#include <optional>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data::parser {

enum class event_type : std::uint32_t;

} // namespace snail::perf_data::parser

namespace snail::perf_data::detail {

std::optional<std::size_t> calculate_id_offset(const parser::sample_format_flags& format);
std::optional<std::size_t> calculate_id_back_offset(const parser::sample_format_flags& format);

struct event_attributes_database
{
    std::vector<parser::event_attributes>                        all_attributes;
    std::unordered_map<std::uint64_t, parser::event_attributes*> id_to_attributes;

    void validate();

    const parser::event_attributes& get_event_attributes(std::endian                byte_order,
                                                         parser::event_type         event_type,
                                                         std::span<const std::byte> event_data) const;

private:
    std::optional<std::size_t> id_offset;
    std::optional<std::size_t> id_back_offset;
};

} // namespace snail::perf_data::detail
