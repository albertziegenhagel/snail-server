
#include <array>
#include <bit>
#include <iostream>

#include "etlfile.hpp"

#include "parser/buffer.hpp"
#include "parser/generic.hpp"
#include "parser/records/kernel/header.hpp"
#include "parser/trace.hpp"
#include "parser/trace_headers/compact_trace.hpp"
#include "parser/trace_headers/event_header_trace.hpp"
#include "parser/trace_headers/full_header_trace.hpp"
#include "parser/trace_headers/instance_trace.hpp"
#include "parser/trace_headers/perfinfo_trace.hpp"
#include "parser/trace_headers/system_trace.hpp"

#include "binarystream/memory_stream.hpp"

using namespace perfreader::etl;
using namespace perfreader::utility;

namespace
{

// All known trace headers use 16bit integers for their buffer sizes,
// hence this size should always be enough.
// inline constexpr std::size_t buffer_size = 0xFFFFU;
inline constexpr std::size_t buffer_size = 0x10000U;

void process_system_trace(binary_stream_parser& parser,
                          std::span<std::byte> buffer,
                          event_observer& callbacks)
{
    parser::system_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto user_data_size = trace_header.packet.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_compact_trace(binary_stream_parser& parser,
                           std::span<std::byte> buffer,
                           event_observer& callbacks)
{
    parser::compact_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto user_data_size = trace_header.packet.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_perfinfo_trace(binary_stream_parser& parser,
                            std::span<std::byte> buffer,
                            event_observer& callbacks)
{
    parser::perfinfo_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto user_data_size = trace_header.packet.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_full_header_trace(binary_stream_parser& parser,
                               std::span<std::byte> buffer,
                               event_observer& callbacks)
{
    parser::full_header_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto user_data_size = trace_header.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_event_header_trace(binary_stream_parser& parser,
                                std::span<std::byte> buffer,
                                event_observer& callbacks)
{
    parser::event_header_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto is_extended = (trace_header.flags & static_cast<std::underlying_type_t<parser::event_header_flag>>(parser::event_header_flag::extended_info)) != 0;

    assert(!is_extended); // not yet supported

    const auto user_data_size = trace_header.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_instance_trace(binary_stream_parser& parser,
                            std::span<std::byte> buffer,
                            event_observer& callbacks)
{
    parser::instance_trace_header trace_header;
    const auto header_size = parser::parse(parser, trace_header);

    const auto user_data_size = trace_header.size - header_size;
    assert(user_data_size <= buffer_size);

    const auto user_data_buffer = buffer.subspan(0, user_data_size);
    parser.read(user_data_buffer);

    callbacks.handle(trace_header, user_data_buffer);
}

void process_next_trace(const parser::wmi_buffer_header& /*buffer_header*/,
                        binary_stream_parser& parser,
                        std::span<std::byte> buffer,
                        event_observer& callbacks)
{
    const auto initial_position = parser.tell_position();

    const auto marker = parser::peek<parser::generic_trace_marker>(parser);
    assert(marker.is_trace_header());
    assert(marker.is_trace_header_event_trace());
    assert(!marker.is_trace_message());

    switch(marker.header_type)
    {
    case parser::trace_header_type::system32:
    case parser::trace_header_type::system64:
        process_system_trace(parser, buffer, callbacks);
        break;
    case parser::trace_header_type::compact32:
    case parser::trace_header_type::compact64:
        process_compact_trace(parser, buffer, callbacks);
        break;
    case parser::trace_header_type::perfinfo32:
    case parser::trace_header_type::perfinfo64:
        process_perfinfo_trace(parser, buffer, callbacks);
        break;
    case parser::trace_header_type::full_header32:
    case parser::trace_header_type::full_header64:
        process_full_header_trace(parser, buffer, callbacks);
        break;
    case parser::trace_header_type::event_header32:
    case parser::trace_header_type::event_header64:
        process_event_header_trace(parser, buffer, callbacks);
        break;
    case parser::trace_header_type::instance32:
    case parser::trace_header_type::instance64:
        process_instance_trace(parser, buffer, callbacks);
        break;
    default:
        throw std::runtime_error(std::format("Unsupported trace header type {}", (int)marker.header_type));
    }

    // Traces are always aligned to 8 byte blocks
    constexpr unsigned int alignment = 8;

    const auto end_position = parser.tell_position();
    const auto read_bytes   = end_position - initial_position;
    const auto misalignment = read_bytes % alignment;
    if(misalignment > 0)
    {
        parser.skip(alignment - misalignment);
    }
}

} // namespace

etl_file::etl_file(const std::filesystem::path& file_path)
{
    open(file_path);
}

void etl_file::open(const std::filesystem::path& file_path)
{
    file_stream_.open(file_path);

    if(!file_stream_.is_open())
    {
        // throw std::runtime_error();
    }

    auto parser = binary_stream_parser(file_stream_, std::endian::little);

    const auto init_position = file_stream_.tellg(); // this should always be 0

    // Read the first buffer
    parser::wmi_buffer_header buffer_header;
    parser::parse(parser, buffer_header);

    assert(buffer_header.buffer_type == parser::etw_buffer_type::header);

    // The first buffer needs to include a system trace header
    const auto marker = parser::peek<parser::generic_trace_marker>(parser);
    assert(marker.is_trace_header() && marker.is_trace_header_event_trace() && !marker.is_trace_message());
    assert(marker.header_type == parser::trace_header_type::system32 ||
           marker.header_type == parser::trace_header_type::system64);

    parser::system_trace_header system_trace_header;
    parser::parse(parser, system_trace_header);

    // the first record needs to be a event-trace-header
    assert(system_trace_header.packet.group == parser::event_trace_group::header);
    assert(system_trace_header.packet.type == 0);
    assert(system_trace_header.version == 2);

    parser::event_trace_header_v2 event_trace_header;
    parser::parse(parser, event_trace_header);

    assert((file_stream_.tellg() - init_position) <= buffer_header.wnode.saved_offset);

    // Reset to beginning of the file. We will process the header event during
    // `etl_file::process()` just like any other event
    file_stream_.seekg(init_position);
}

void etl_file::close()
{
    file_stream_.close();
}

void etl_file::process(event_observer& callbacks)
{
    std::array<std::byte, buffer_size> user_data_buffer;

    auto parser = binary_stream_parser(file_stream_, std::endian::little);
    
    // read all buffers
    while(file_stream_.good() && file_stream_.peek() != EOF)
    {
        const auto init_position = file_stream_.tellg();

        parser::wmi_buffer_header buffer_header;
        const auto header_size = parser::parse(parser, buffer_header);

        const auto payload_size     = buffer_header.wnode.saved_offset - header_size;
        const auto payload_position = file_stream_.tellg();

        // read traces as long as there is payload data left to read
        auto payload_read_bytes = file_stream_.tellg() - payload_position;
        while(payload_read_bytes < static_cast<std::streamoff>(payload_size))
        {
            process_next_trace(buffer_header, parser, user_data_buffer, callbacks);
            payload_read_bytes = file_stream_.tellg() - payload_position;
        }
        assert(payload_read_bytes == static_cast<std::streamoff>(payload_size));

        // skip the padding at the end of this buffer
        const auto total_read_bytes = file_stream_.tellg() - init_position;
        assert(total_read_bytes == buffer_header.wnode.saved_offset);
        parser.skip(buffer_header.wnode.buffer_size - total_read_bytes);
    }

    // std::array<std::byte, buffer_size> buffer;
    // std::array<std::byte, buffer_size> user_data_buffer;

    // // read all buffers
    // while(file_stream_.good() && file_stream_.peek() != EOF)
    // {
    //     const auto init_position = file_stream_.tellg();

    //     file_stream_.read(reinterpret_cast<char*>(buffer.data()), buffer_size);
    //     auto buffer_stream = memory_istream(std::span(buffer));
    //     auto parser = binary_stream_parser(buffer_stream, std::endian::little);

    //     parser::wmi_buffer_header buffer_header;
    //     const auto header_size = parser::parse(parser, buffer_header);

    //     assert(buffer_header.wnode.buffer_size <= buffer_size);

    //     const auto payload_size     = buffer_header.wnode.saved_offset - header_size;
    //     const auto payload_position = buffer_stream.tellg();

    //     // read traces as long as there is payload data left to read
    //     auto payload_read_bytes = buffer_stream.tellg() - payload_position;
    //     while(payload_read_bytes < static_cast<std::streamoff>(payload_size))
    //     {
    //         process_next_trace(buffer_header, parser, user_data_buffer, callbacks);
    //         payload_read_bytes = buffer_stream.tellg() - payload_position;
    //     }
    //     assert(payload_read_bytes == static_cast<std::streamoff>(payload_size));

    //     if(buffer_header.wnode.buffer_size < buffer_size)
    //     {
    //         file_stream_.seekg(init_position + std::streamoff(buffer_header.wnode.buffer_size));
    //     }

    //     // // skip the padding at the end of this buffer
    //     // const auto total_read_bytes = parser.tell_position() - init_position;
    //     // // assert(total_read_bytes == buffer_header.wnode.saved_offset);
    //     // parser.skip(buffer_header.wnode.buffer_size - total_read_bytes);
    // }

    // // read all buffers
    // while(parser.has_more_data())
    // {
    //     const auto init_position = file_stream_.tellg();

    //     parser::wmi_buffer_header buffer_header;
    //     const auto header_size = parser::parse(parser, buffer_header);

    //     const auto payload_size     = buffer_header.wnode.saved_offset - header_size;
    //     const auto payload_position = file_stream_.tellg();

    //     // read traces as long as there is payload data left to read
    //     auto payload_read_bytes = file_stream_.tellg() - payload_position;
    //     while(payload_read_bytes < static_cast<std::streamoff>(payload_size))
    //     {
    //         process_next_trace(buffer_header, parser, buffer, callbacks);
    //         payload_read_bytes = file_stream_.tellg() - payload_position;
    //     }
    //     assert(payload_read_bytes == static_cast<std::streamoff>(payload_size));

    //     // skip the padding at the end of this buffer
    //     const auto total_read_bytes = file_stream_.tellg() - init_position;
    //     // assert(total_read_bytes == buffer_header.wnode.saved_offset);
    //     parser.skip(buffer_header.wnode.buffer_size - total_read_bytes);
    // }
}
