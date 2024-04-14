
#include <vector>

#include <snail/perf_data/parser/records/kernel.hpp>

using namespace snail::common::parser;
using namespace snail::perf_data::parser;

sample_id snail::perf_data::parser::parse_sample_id(const sample_format_flags& sample_format,
                                                    std::span<const std::byte> buffer,
                                                    std::endian                byte_order)
{
    sample_id result;

    std::size_t offset = 0;
    result.pid         = extract_move_if<std::uint32_t>(sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);
    result.tid         = extract_move_if<std::uint32_t>(sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);
    result.time        = extract_move_if<std::uint64_t>(sample_format.test(parser::sample_format::time), buffer, offset, byte_order);
    result.id          = extract_move_if<std::uint64_t>(sample_format.test(parser::sample_format::id), buffer, offset, byte_order);
    result.stream_id   = extract_move_if<std::uint64_t>(sample_format.test(parser::sample_format::stream_id), buffer, offset, byte_order);
    result.cpu         = extract_move_if<std::uint32_t>(sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.res         = extract_move_if<std::uint32_t>(sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.id          = extract_move_if<std::uint64_t>(sample_format.test(parser::sample_format::identifier), buffer, offset, byte_order);

    return result;
}

sample_id snail::perf_data::parser::parse_sample_id_back(const sample_format_flags& sample_format,
                                                         std::span<const std::byte> buffer,
                                                         std::endian                byte_order)
{
    sample_id result;

    std::size_t offset = buffer.size();
    result.id          = extract_move_back_if<std::uint64_t>(sample_format.test(parser::sample_format::identifier), buffer, offset, byte_order);
    result.res         = extract_move_back_if<std::uint32_t>(sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.cpu         = extract_move_back_if<std::uint32_t>(sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.stream_id   = extract_move_back_if<std::uint64_t>(sample_format.test(parser::sample_format::stream_id), buffer, offset, byte_order);
    result.id          = extract_move_back_if<std::uint64_t>(sample_format.test(parser::sample_format::id), buffer, offset, byte_order);
    result.time        = extract_move_back_if<std::uint64_t>(sample_format.test(parser::sample_format::time), buffer, offset, byte_order);
    result.tid         = extract_move_back_if<std::uint32_t>(sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);
    result.pid         = extract_move_back_if<std::uint32_t>(sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);

    return result;
}

template<>
sample_event snail::perf_data::parser::parse_event(const event_attributes&    attributes,
                                                   std::span<const std::byte> buffer,
                                                   std::endian                byte_order)
{
    sample_event result;

    std::size_t offset = event_header_view::static_size;

    result.attributes = &attributes;

    result.id        = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::identifier), buffer, offset, byte_order);
    result.ip        = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::ip), buffer, offset, byte_order);
    result.pid       = extract_move_if<std::uint32_t>(attributes.sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);
    result.tid       = extract_move_if<std::uint32_t>(attributes.sample_format.test(parser::sample_format::tid), buffer, offset, byte_order);
    result.time      = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::time), buffer, offset, byte_order);
    result.addr      = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::addr), buffer, offset, byte_order);
    result.id        = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::id), buffer, offset, byte_order);
    result.stream_id = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::stream_id), buffer, offset, byte_order);
    result.cpu       = extract_move_if<std::uint32_t>(attributes.sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.res       = extract_move_if<std::uint32_t>(attributes.sample_format.test(parser::sample_format::cpu), buffer, offset, byte_order);
    result.period    = extract_move_if<std::uint64_t>(attributes.sample_format.test(parser::sample_format::period), buffer, offset, byte_order);

    assert(!attributes.sample_format.test(parser::sample_format::read)); // Not yet supported

    if(attributes.sample_format.test(parser::sample_format::call_chain))
    {
        const auto                 size = extract_move<std::uint64_t>(buffer, offset, byte_order);
        std::vector<std::uint64_t> ips(static_cast<std::size_t>(size));
        for(auto& ip : ips)
        {
            ip = extract_move<std::uint64_t>(buffer, offset, byte_order);
        }
        result.ips = std::move(ips);
    }

    if(attributes.sample_format.test(parser::sample_format::raw))
    {
        const auto                size = extract_move<std::uint64_t>(buffer, offset, byte_order);
        std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
        for(auto& entry : data)
        {
            entry = extract_move<std::uint8_t>(buffer, offset, byte_order);
        }
        result.data = std::move(data);
    }

    assert(offset == buffer.size()); // Remaining fields not yet supported

    return result;
}
