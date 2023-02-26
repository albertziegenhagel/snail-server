
#include <snail/perf_data/detail/attributes_database.hpp>

#include <cassert>
#include <iostream>

#include <snail/perf_data/parser/event.hpp>

#include <snail/perf_data/detail/file_header.hpp>

using namespace snail::perf_data;
using namespace snail::perf_data::detail;

void event_attributes_database::validate()
{
    if(all_attributes.empty())
    {
        throw std::runtime_error("Invalid perf.data file: empty event attributes");
    }

    const auto& main_attributes = all_attributes.front();

    if(main_attributes.sample_format.test(parser::sample_format::read) &&
       !main_attributes.read_format.test(parser::read_format::id))
    {
        throw std::runtime_error("Invalid perf.data file: non-matching read format");
    }

    id_offset      = calculate_id_offset(main_attributes.sample_format);
    id_back_offset = calculate_id_back_offset(main_attributes.sample_format);

    for(const auto& attributes : all_attributes)
    {
        if(id_offset != calculate_id_offset(attributes.sample_format) ||
           id_back_offset != calculate_id_back_offset(attributes.sample_format))
        {
            throw std::runtime_error("Invalid perf.data file: non-matching id positions");
        }

        if(main_attributes.flags.test(parser::attribute_flag::sample_id_all) !=
           attributes.flags.test(parser::attribute_flag::sample_id_all))
        {
            throw std::runtime_error("Invalid perf.data file: non-matching sample_id_all");
        }

        if(main_attributes.read_format.data() !=
           attributes.read_format.data())
        {
            throw std::runtime_error("Invalid perf.data file: non-matching read_format");
        }
    }
}

std::optional<std::size_t> event_attributes_database::calculate_id_offset(const parser::sample_format_flags& format)
{
    if(format.test(parser::sample_format::identifier)) return 0;
    if(!format.test(parser::sample_format::id)) return std::nullopt;

    std::size_t offset = 0;

    if(format.test(parser::sample_format::ip)) offset += 8;
    if(format.test(parser::sample_format::tid)) offset += 8;
    if(format.test(parser::sample_format::time)) offset += 8;
    if(format.test(parser::sample_format::addr)) offset += 8;

    return offset;
}

std::optional<std::size_t> event_attributes_database::calculate_id_back_offset(const parser::sample_format_flags& format)
{
    if(format.test(parser::sample_format::identifier)) return 8;
    if(!format.test(parser::sample_format::id)) return std::nullopt;

    std::size_t offset = 8;

    if(format.test(parser::sample_format::cpu)) offset += 8;
    if(format.test(parser::sample_format::stream_id)) offset += 8;

    return offset;
}

const parser::event_attributes& event_attributes_database::get_event_attributes(
    const detail::perf_data_file_header_data& file_header,
    const parser::event_header_view&          event_header,
    std::span<const std::byte>                event_data) const
{
    assert(!all_attributes.empty());

    const auto& main_attributes = all_attributes.front();

    if(all_attributes.size() == 1)
    {
        return main_attributes;
    }

    if(!main_attributes.flags.test(parser::attribute_flag::sample_id_all) &&
       event_header.type() != parser::event_type::sample)
    {
        return main_attributes;
    }

    const auto id = event_header.type() == parser::event_type::sample ?
                        common::parser::extract<std::uint64_t>(event_data, *id_offset, file_header.byte_order) :
                        common::parser::extract<std::uint64_t>(event_data, event_data.size() - *id_back_offset, file_header.byte_order);

    if(id == 0)
    {
        return main_attributes;
    }

    const auto iter = id_to_attributes.find(id);
    if(iter == id_to_attributes.end())
    {
        throw std::runtime_error("Could not find event attributes for ID");
    }
    return *iter->second;
}
