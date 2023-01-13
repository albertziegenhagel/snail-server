
#include <array>
#include <bit>
#include <iostream>

#include "etlfile.hpp"

#include "parser/buffer.hpp"
#include "parser/records/kernel/header.hpp"
#include "parser/trace.hpp"
#include "parser/trace_headers/compact_trace.hpp"
#include "parser/trace_headers/event_header_trace.hpp"
#include "parser/trace_headers/full_header_trace.hpp"
#include "parser/trace_headers/instance_trace.hpp"
#include "parser/trace_headers/perfinfo_trace.hpp"
#include "parser/trace_headers/system_trace.hpp"

using namespace snail::etl;

namespace
{

// All known trace headers use 16bit integers for their buffer sizes,
// hence this size should always be enough.
// inline constexpr std::size_t buffer_size = 0xFFFFU;
inline constexpr std::size_t max_buffer_size = 0x10000U;

// Supports:
//    parser::system_trace_header_view,
//    parser::compact_trace_header_view and
//    parser::perfinfo_trace_header_view
template<typename TraceHeaderViewType>
std::size_t process_type_1_trace(std::span<const std::byte> payload_buffer,
                                 const etl_file::header_data& file_header,
                                 event_observer& callbacks)
{
    const auto trace_header = TraceHeaderViewType(payload_buffer);

    assert(trace_header.packet().size() >= TraceHeaderViewType::static_size);
    const auto user_data_size = trace_header.packet().size() - TraceHeaderViewType::static_size;
    assert(user_data_size <= max_buffer_size);

    const auto user_data_buffer = payload_buffer.subspan(TraceHeaderViewType::static_size, user_data_size);

    callbacks.handle(file_header, trace_header, user_data_buffer);

    return static_cast<std::size_t>(trace_header.packet().size());
}

// Supports:
//    parser::full_header_trace_header_view and
//    parser::instance_trace_header_view
template<typename TraceHeaderViewType>
std::size_t process_type_2_trace(std::span<const std::byte> payload_buffer,
                                 const etl_file::header_data& file_header,
                                 event_observer& callbacks)
{
    const auto trace_header = TraceHeaderViewType(payload_buffer);

    assert(trace_header.size() >= TraceHeaderViewType::static_size);
    const auto user_data_size = trace_header.size() - TraceHeaderViewType::static_size;
    assert(user_data_size <= max_buffer_size);

    const auto user_data_buffer = payload_buffer.subspan(TraceHeaderViewType::static_size, user_data_size);

    callbacks.handle(file_header, trace_header, user_data_buffer);

    return static_cast<std::size_t>(trace_header.size());
}

// Could basically be handled by `process_type_2_trace`, but `event_header_trace_header_view`
// can have extended data.
std::size_t process_event_header_trace(std::span<const std::byte> payload_buffer,
                                       const etl_file::header_data& file_header,
                                       event_observer& callbacks)
{
    const auto trace_header = parser::event_header_trace_header_view(payload_buffer);

    const auto is_extended = (trace_header.flags() & static_cast<std::underlying_type_t<parser::event_header_flag>>(parser::event_header_flag::extended_info)) != 0;

    assert(!is_extended); // not yet supported

    assert(trace_header.size() >= parser::event_header_trace_header_view::static_size);
    const auto user_data_size = trace_header.size() - parser::event_header_trace_header_view::static_size;
    assert(user_data_size <= max_buffer_size);

    const auto user_data_buffer = payload_buffer.subspan(parser::event_header_trace_header_view::static_size, user_data_size);

    callbacks.handle(file_header, trace_header, user_data_buffer);

    return static_cast<std::size_t>(trace_header.size());
}

std::size_t process_next_trace(std::span<const std::byte> payload_buffer,
                               const etl_file::header_data& file_header,
                               event_observer& callbacks)
{
    const auto marker = parser::generic_trace_marker_view(payload_buffer);
    assert(marker.is_trace_header() && marker.is_trace_header_event_trace() && !marker.is_trace_message());
           
    std::size_t read_bytes = 0;
    switch(marker.header_type())
    {
    case parser::trace_header_type::system32:
    case parser::trace_header_type::system64:
        read_bytes = process_type_1_trace<parser::system_trace_header_view>(payload_buffer, file_header, callbacks);
        break;
    case parser::trace_header_type::compact32:
    case parser::trace_header_type::compact64:
        read_bytes = process_type_1_trace<parser::compact_trace_header_view>(payload_buffer, file_header, callbacks);
        break;
    case parser::trace_header_type::perfinfo32:
    case parser::trace_header_type::perfinfo64:
        read_bytes = process_type_1_trace<parser::perfinfo_trace_header_view>(payload_buffer, file_header, callbacks);
        break;
    case parser::trace_header_type::full_header32:
    case parser::trace_header_type::full_header64:
        read_bytes = process_type_2_trace<parser::full_header_trace_header_view>(payload_buffer, file_header, callbacks);
        break;
    case parser::trace_header_type::instance32:
    case parser::trace_header_type::instance64:
        read_bytes = process_type_2_trace<parser::instance_trace_header_view>(payload_buffer, file_header, callbacks);
        break;
    case parser::trace_header_type::event_header32:
    case parser::trace_header_type::event_header64:
        read_bytes = process_event_header_trace(payload_buffer, file_header, callbacks);
        break;
    default:
        throw std::runtime_error(std::format("Unsupported trace header type {}", (int)marker.header_type()));
    }

    // Traces are always aligned to 8 byte blocks
    constexpr std::size_t alignment = 8;

    const auto misalignment = read_bytes % alignment;
    if(misalignment > 0)
    {
        read_bytes += alignment - misalignment;
    }

    return read_bytes;
}

} // namespace

etl_file::etl_file(const std::filesystem::path& file_path)
{
    open(file_path);
}

void etl_file::open(const std::filesystem::path& file_path)
{
    file_stream_.open(file_path, std::ios_base::binary);

    if(!file_stream_.is_open())
    {
        std::cout << "ERROR: could not open file" << file_path << std::endl;
        return; // TODO: handle error
    }
    std::array<std::byte, max_buffer_size> file_buffer_data;

    file_stream_.read(reinterpret_cast<char*>(file_buffer_data.data()), file_buffer_data.size());

    const auto read_bytes = file_stream_.tellg() - std::streampos(0);
    if(read_bytes < parser::wmi_buffer_header_view::static_size)
    {
        std::cout << "ERROR: Invalid ETL file: insufficient size for buffer header" << std::endl;
        return; // TODO: handle error
    }

    const auto file_buffer = std::span(file_buffer_data);

    const auto buffer_header = parser::wmi_buffer_header_view(file_buffer);

    assert(buffer_header.buffer_type() == parser::etw_buffer_type::header);

    if(read_bytes < buffer_header.wnode().saved_offset())
    {
        std::cout << "ERROR: Invalid ETL file: insufficient size for buffer" << std::endl;
        return; // TODO: handle error
    }

    const auto marker = parser::generic_trace_marker_view(file_buffer.subspan(parser::wmi_buffer_header_view::static_size));
    assert(marker.is_trace_header() && marker.is_trace_header_event_trace() && !marker.is_trace_message());
    assert(marker.header_type() == parser::trace_header_type::system32 ||
           marker.header_type() == parser::trace_header_type::system64);
           
    const auto system_trace_header = parser::system_trace_header_view(file_buffer.subspan(
        parser::wmi_buffer_header_view::static_size));

    // the first record needs to be a event-trace-header
    assert(system_trace_header.packet().group() == parser::event_trace_group::header);
    assert(system_trace_header.packet().type() == 0);
    assert(system_trace_header.version() == 2);

    const auto event_trace_header = parser::event_trace_v2_header_event_view(file_buffer.subspan(
        parser::wmi_buffer_header_view::static_size +
        parser::system_trace_header_view::static_size));

    header_ = etl_file::header_data{
        .pointer_size=event_trace_header.pointer_size()
        // TODO: extract more (all?) relevant data
    };

    file_stream_.seekg(0);
}

void etl_file::close()
{
    file_stream_.close();
}

void etl_file::process(event_observer& callbacks)
{
    std::array<std::byte, max_buffer_size> file_buffer_data;

    // read all buffers
    for(std::size_t buffer_index = 0; file_stream_.good() && file_stream_.peek() != EOF; ++buffer_index)
    {
        const auto init_position = file_stream_.tellg();

        file_stream_.read(reinterpret_cast<char*>(file_buffer_data.data()), file_buffer_data.size());

        const auto read_bytes = file_stream_.tellg() - init_position;
        if(read_bytes < parser::wmi_buffer_header_view::static_size)
        {
            std::cout << std::format(
                "ERROR: Invalid ETL file: insufficient size for buffer header (index {}). Expected {} but read only {}.",
                buffer_index,
                parser::wmi_buffer_header_view::static_size,
                read_bytes) << std::endl;
            return; // TODO: handle error
        }

        const auto file_buffer = std::span(file_buffer_data);

        const auto buffer_header = parser::wmi_buffer_header_view(file_buffer);

        // assert(buffer_header.buffer_type() == parser::etw_buffer_type::header);

        if(buffer_header.wnode().buffer_size() > file_buffer_data.size())
        {
            std::cout << std::format(
                "ERROR: Unsupported ETL file: buffer size ({}) exceeds maximum buffer size ({})",
                buffer_header.wnode().buffer_size(),
                file_buffer.size()) << std::endl;
            return; // TODO: handle error
        }
        if(read_bytes < buffer_header.wnode().saved_offset())
        {
            std::cout << std::format(
                "ERROR: Invalid ETL file: insufficient size for buffer (index {}). Expected {} but read only {}.",
                buffer_index,
                buffer_header.wnode().saved_offset(),
                read_bytes) << std::endl;
            return; // TODO: handle error
        }

        const auto payload_size   = buffer_header.wnode().saved_offset() - parser::wmi_buffer_header_view::static_size;
        const auto payload_buffer = file_buffer.subspan(parser::wmi_buffer_header_view::static_size, payload_size);

        // read traces as long as there is payload data left to read
        std::size_t payload_offset = 0;
        while(payload_offset < payload_size)
        {
            const auto trace_read_bytes = process_next_trace(payload_buffer.subspan(payload_offset), header_, callbacks);
            payload_offset += trace_read_bytes;
        }

        // If we have read to much data, reset the file pointer to the end of the current buffer
        // We expect that usually the actual buffer size will exactly match the max buffer size, hence
        // we do not expect this branch to be taken. If our assumption is incorrect, this would actually
        // be very inefficient.
        if(buffer_header.wnode().buffer_size() < max_buffer_size)
        {
            file_stream_.seekg(init_position + std::streamoff(buffer_header.wnode().buffer_size()));
        }
    }
}
