
#include <snail/etl/etl_file.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <format>
#include <iostream>
#include <queue>
#include <stdexcept>

#include <snail/common/cast.hpp>

#include <snail/etl/parser/buffer.hpp>
#include <snail/etl/parser/records/kernel/header.hpp>
#include <snail/etl/parser/trace.hpp>
#include <snail/etl/parser/trace_headers/compact_trace.hpp>
#include <snail/etl/parser/trace_headers/event_header_trace.hpp>
#include <snail/etl/parser/trace_headers/full_header_trace.hpp>
#include <snail/etl/parser/trace_headers/instance_trace.hpp>
#include <snail/etl/parser/trace_headers/perfinfo_trace.hpp>
#include <snail/etl/parser/trace_headers/system_trace.hpp>

using namespace snail::common;
using namespace snail::etl;

namespace {

// All known trace headers use 16bit integers for their buffer sizes,
// hence this size should always be enough.
inline constexpr std::size_t max_buffer_size = 0xFFFF + 1;

struct buffer_info
{
    std::span<const std::byte> header_buffer;
    std::span<const std::byte> payload_buffer;
    std::size_t                current_payload_offset;
};

struct processor_data
{
    std::array<std::byte, max_buffer_size> current_buffer_data;

    buffer_info current_buffer_info;

    struct file_buffer_info
    {
        std::streampos start_pos;
        std::int64_t   sequence_number;
    };

    std::vector<file_buffer_info> remaining_buffers;
};

struct next_event_priority_info
{
    std::uint64_t next_event_time;
    std::size_t   processor_index;

    bool operator<(const next_event_priority_info& other) const
    {
        // event with lowest timestamp has highest priority
        return next_event_time > other.next_event_time;
    }
};

buffer_info read_buffer(std::ifstream&                          file_stream,
                        std::streampos                          buffer_start_pos,
                        std::array<std::byte, max_buffer_size>& buffer_data)
{
    file_stream.seekg(buffer_start_pos);
    file_stream.read(reinterpret_cast<char*>(buffer_data.data()), narrow_cast<std::streamsize>(buffer_data.size()));

    const auto read_bytes = file_stream.tellg() - buffer_start_pos;
    if(read_bytes < static_cast<std::streamoff>(parser::wmi_buffer_header_view::static_size))
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: insufficient size for buffer header. Expected {} but read only {}.",
            parser::wmi_buffer_header_view::static_size,
            read_bytes));
    }
    const auto total_buffer = std::span(buffer_data);

    const auto header_buffer = total_buffer.subspan(0, parser::wmi_buffer_header_view::static_size);

    const auto header = parser::wmi_buffer_header_view(header_buffer);

    if(header.wnode().buffer_size() > total_buffer.size())
    {
        throw std::runtime_error(std::format(
            "Unsupported ETL file: buffer size ({}) exceeds maximum buffer size ({})",
            header.wnode().buffer_size(),
            total_buffer.size()));
    }
    if(read_bytes < header.wnode().saved_offset())
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: insufficient size for buffer. Expected {} but read only {}.",
            header.wnode().saved_offset(),
            read_bytes));
    }

    const auto payload_size   = header.wnode().saved_offset() - parser::wmi_buffer_header_view::static_size;
    const auto payload_buffer = total_buffer.subspan(parser::wmi_buffer_header_view::static_size, payload_size);

    return buffer_info{header_buffer, payload_buffer, 0};
}

std::uint64_t peak_next_event_time(const buffer_info& buffer)
{
    const auto event_buffer = buffer.payload_buffer.subspan(buffer.current_payload_offset);
    const auto marker       = parser::generic_trace_marker_view(event_buffer);
    assert(marker.is_trace_header() && marker.is_trace_header_event_trace() && !marker.is_trace_message());

    switch(marker.header_type())
    {
    case parser::trace_header_type::system32:
    case parser::trace_header_type::system64:
        return parser::system_trace_header_view(event_buffer).system_time();
    case parser::trace_header_type::compact32:
    case parser::trace_header_type::compact64:
        return parser::compact_trace_header_view(event_buffer).system_time();
    case parser::trace_header_type::perfinfo32:
    case parser::trace_header_type::perfinfo64:
        return parser::perfinfo_trace_header_view(event_buffer).system_time();
    case parser::trace_header_type::full_header32:
    case parser::trace_header_type::full_header64:
        return parser::full_header_trace_header_view(event_buffer).timestamp();
    case parser::trace_header_type::instance32:
    case parser::trace_header_type::instance64:
        return parser::instance_trace_header_view(event_buffer).timestamp();
    case parser::trace_header_type::event_header32:
    case parser::trace_header_type::event_header64:
        return parser::event_header_trace_header_view(event_buffer).timestamp();
    default:
        throw std::runtime_error(std::format("Unsupported trace header type {}", (int)marker.header_type()));
    }
}

// Supports:
//    parser::system_trace_header_view,
//    parser::compact_trace_header_view and
//    parser::perfinfo_trace_header_view
template<typename TraceHeaderViewType>
std::size_t process_type_1_trace(std::span<const std::byte>   payload_buffer,
                                 const etl_file::header_data& file_header,
                                 event_observer&              callbacks)
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
std::size_t process_type_2_trace(std::span<const std::byte>   payload_buffer,
                                 const etl_file::header_data& file_header,
                                 event_observer&              callbacks)
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
std::size_t process_event_header_trace(std::span<const std::byte>   payload_buffer,
                                       const etl_file::header_data& file_header,
                                       event_observer&              callbacks)
{
    const auto trace_header = parser::event_header_trace_header_view(payload_buffer);

    [[maybe_unused]] const auto is_extended = (trace_header.flags() & static_cast<std::underlying_type_t<parser::event_header_flag>>(parser::event_header_flag::extended_info)) != 0;

    assert(!is_extended); // not yet supported

    assert(trace_header.size() >= parser::event_header_trace_header_view::static_size);
    const auto user_data_size = trace_header.size() - parser::event_header_trace_header_view::static_size;
    assert(user_data_size <= max_buffer_size);

    const auto user_data_buffer = payload_buffer.subspan(parser::event_header_trace_header_view::static_size, user_data_size);

    callbacks.handle(file_header, trace_header, user_data_buffer);

    return static_cast<std::size_t>(trace_header.size());
}

std::size_t process_next_trace(std::span<const std::byte>   payload_buffer,
                               const etl_file::header_data& file_header,
                               event_observer&              callbacks)
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
        throw std::runtime_error(std::format("Could not open file {}", file_path.string()));
    }
    std::array<std::byte, max_buffer_size> file_buffer_data;

    file_stream_.read(reinterpret_cast<char*>(file_buffer_data.data()), file_buffer_data.size());

    const auto read_bytes = file_stream_.tellg() - std::streampos(0);
    if(read_bytes < static_cast<std::streamoff>(parser::wmi_buffer_header_view::static_size))
    {
        throw std::runtime_error("Invalid ETL file: insufficient size for buffer header");
    }

    const auto file_buffer = std::span(file_buffer_data);

    const auto buffer_header = parser::wmi_buffer_header_view(file_buffer);

    assert(buffer_header.buffer_type() == parser::etw_buffer_type::header);

    if(read_bytes < buffer_header.wnode().saved_offset())
    {
        throw std::runtime_error("Invalid ETL file: insufficient size for buffer");
    }

    [[maybe_unused]] const auto marker = parser::generic_trace_marker_view(file_buffer.subspan(parser::wmi_buffer_header_view::static_size));
    assert(marker.is_trace_header() && marker.is_trace_header_event_trace() && !marker.is_trace_message());
    assert(marker.header_type() == parser::trace_header_type::system32 ||
           marker.header_type() == parser::trace_header_type::system64);

    const auto system_trace_header = parser::system_trace_header_view(file_buffer.subspan(
        parser::wmi_buffer_header_view::static_size));

    // the first record needs to be a event-trace-header
    assert(system_trace_header.packet().group() == parser::event_trace_group::header);
    assert(system_trace_header.packet().type() == 0);
    assert(system_trace_header.version() == 2);

    const auto header_event = parser::event_trace_v2_header_event_view(file_buffer.subspan(
        parser::wmi_buffer_header_view::static_size +
        parser::system_trace_header_view::static_size));

    header_ = etl_file::header_data{
        .start_time           = common::from_nt_timestamp(common::nt_duration(header_event.start_time())),
        .end_time             = common::from_nt_timestamp(common::nt_duration(header_event.end_time())),
        .start_time_qpc_ticks = system_trace_header.system_time(),
        .qpc_frequency        = header_event.perf_freq(),
        .pointer_size         = header_event.pointer_size(),
        .number_of_processors = header_event.number_of_processors(),
        .number_of_buffers    = header_event.buffers_written()
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
    std::vector<processor_data> per_processor_data{header_.number_of_processors};

    // build a list of file buffers per processor
    {
        std::array<std::byte, parser::wmi_buffer_header_view::static_size> header_buffer_data;
        for(std::size_t buffer_index = 0; buffer_index < header_.number_of_buffers; ++buffer_index)
        {
            const auto init_position = file_stream_.tellg();

            file_stream_.read(reinterpret_cast<char*>(header_buffer_data.data()), header_buffer_data.size());

            assert(file_stream_.good());

            const auto read_bytes = file_stream_.tellg() - init_position;
            if(read_bytes < static_cast<std::streamoff>(parser::wmi_buffer_header_view::static_size))
            {
                throw std::runtime_error(std::format(
                    "Invalid ETL file: insufficient size for buffer header (index {}). Expected {} but read only {}.",
                    buffer_index,
                    parser::wmi_buffer_header_view::static_size,
                    read_bytes));
            }
            const auto buffer_header = parser::wmi_buffer_header_view(std::span(header_buffer_data));

            const auto sequence_number = buffer_header.wnode().sequence_number();

            if(sequence_number > header_.number_of_buffers)
            {
                throw std::runtime_error(std::format(
                    "Invalid ETL file: invalid header sequence value. Expected at max {} buffers but sequence number is {}.",
                    header_.number_of_buffers,
                    sequence_number));
            }

            const auto processor_index = buffer_header.wnode().client_context().processor_index();

            if(processor_index > header_.number_of_processors)
            {
                throw std::runtime_error(std::format(
                    "Invalid ETL file: invalid processor index. Expected at max {} processors but process index is {}.",
                    header_.number_of_processors,
                    processor_index));
            }

            per_processor_data[processor_index].remaining_buffers.push_back(
                processor_data::file_buffer_info{
                    .start_pos       = init_position,
                    .sequence_number = sequence_number});

            // seek to start of next buffer
            file_stream_.seekg(init_position + std::streamoff(buffer_header.wnode().buffer_size()));
        }
    }

    // Sort the buffers per processor by their sequence number and read the first buffer
    // for each process.
    // Then extract the time of the first event in each of the processor buffers and initialize
    // a priority queue with the first event times per processor.
    std::priority_queue<next_event_priority_info> event_queue;
    for(std::size_t processor_index = 0; processor_index < per_processor_data.size(); ++processor_index)
    {
        auto& processor_data    = per_processor_data[processor_index];
        auto& remaining_buffers = processor_data.remaining_buffers;

        if(remaining_buffers.empty()) continue;

        std::ranges::sort(remaining_buffers, [](const processor_data::file_buffer_info& lhs, const processor_data::file_buffer_info& rhs)
                          { return lhs.sequence_number > rhs.sequence_number; });

        processor_data.current_buffer_info = read_buffer(file_stream_,
                                                         remaining_buffers.back().start_pos,
                                                         processor_data.current_buffer_data);
        remaining_buffers.pop_back();

        event_queue.push(next_event_priority_info{
            .next_event_time = peak_next_event_time(processor_data.current_buffer_info),
            .processor_index = processor_index});
    }

    // Extract all events in the correct time order:
    //   - Retrieve the processor index that has the event with the lowest time stamp
    //   - Extract all events of that that process until the next event of that process has a later
    //     timestamp than the earliest event of any other processor.
    //   - If any processors buffer is exhausted after an event extraction, we will try to load
    //     the next buffer for that processor (if there are any buffers left).
    while(!event_queue.empty())
    {
        // Get the processor that has the earliest event to extract next.
        const auto [_, processor_index] = event_queue.top();
        event_queue.pop();

        auto& processor_data = per_processor_data[processor_index];

        // Extract events for this processor as long as possible
        while(true)
        {
            auto& buffer_info = processor_data.current_buffer_info;

            // Extract and process the next event
            const auto trace_read_bytes = process_next_trace(buffer_info.payload_buffer.subspan(buffer_info.current_payload_offset), header_, callbacks);
            buffer_info.current_payload_offset += trace_read_bytes;

            // Check whether this was the last event in the current processors buffer
            const auto buffer_exhausted = buffer_info.current_payload_offset >= buffer_info.payload_buffer.size();
            if(buffer_exhausted)
            {
                callbacks.handle(header_, parser::wmi_buffer_header_view(buffer_info.header_buffer));

                auto& remaining_buffers = processor_data.remaining_buffers;

                // If there are no remaining buffers for this processor, stop extracting events for it.
                if(remaining_buffers.empty()) break;

                // Read the next buffer for this processor.
                // Keep in mind, that `remaining_buffers` is sorted.
                buffer_info = read_buffer(file_stream_,
                                          remaining_buffers.back().start_pos,
                                          processor_data.current_buffer_data);
                remaining_buffers.pop_back();
            }

            const auto next_event_time = peak_next_event_time(buffer_info);

            if(!event_queue.empty() && event_queue.top().next_event_time < next_event_time)
            {
                // There is another processor which's next event is earlier than the
                // next event for the current processor. Hence, insert the current processors
                // event into the priority queue and stop extracting events for the current
                // processor.
                event_queue.push(next_event_priority_info{
                    .next_event_time = next_event_time,
                    .processor_index = processor_index});
                break;
            }
        }
    }
}

const etl_file::header_data& etl_file::header() const
{
    return header_;
}
