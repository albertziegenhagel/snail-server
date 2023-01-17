
#pragma once

#include <type_traits>
#include <vector>
#include <unordered_map>
#include <optional>
#include <span>
#include <bit>

#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data::parser {

struct event_header_view;

} // snail::perf_data::parser

namespace snail::perf_data::detail {

struct perf_data_file_header_data;

struct event_attributes_database
{
    std::vector<parser::event_attributes> all_attributes;
    std::unordered_map<std::uint64_t, parser::event_attributes*> id_to_attributes;

    void validate();

    const parser::event_attributes& get_event_attributes(const detail::perf_data_file_header_data& file_header,
                                                         const parser::event_header_view& event_header,
                                                         std::span<const std::byte> event_data) const;

private:
    std::optional<std::size_t> id_offset;
    std::optional<std::size_t> id_back_offset;

    std::optional<std::size_t> calculate_id_offset(const parser::sample_format_flags& format);
    std::optional<std::size_t> calculate_id_back_offset(const parser::sample_format_flags& format);
};


} // namespace snail::perf_data::detail
