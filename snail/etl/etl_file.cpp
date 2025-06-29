
#include <snail/etl/etl_file.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <format>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <utility>

#include <snail/common/cast.hpp>
#include <snail/common/ms_xca_decompression.hpp>

#include <snail/etl/parser/buffer.hpp>
#include <snail/etl/parser/records/kernel/header.hpp>
#include <snail/etl/parser/trace.hpp>
#include <snail/etl/parser/trace_headers/compact_trace.hpp>
#include <snail/etl/parser/trace_headers/event_header_trace.hpp>
#include <snail/etl/parser/trace_headers/full_header_trace.hpp>
#include <snail/etl/parser/trace_headers/instance_trace.hpp>
#include <snail/etl/parser/trace_headers/perfinfo_trace.hpp>
#include <snail/etl/parser/trace_headers/system_trace.hpp>

using namespace snail;
using namespace snail::etl;

namespace {

struct buffer_info
{
    std::span<const std::byte> header_buffer;
    std::span<const std::byte> payload_buffer;
    std::size_t                current_payload_offset;
};

struct processor_data
{
    std::vector<std::byte> current_buffer_data;

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

buffer_info read_buffer(std::ifstream&               file_stream,
                        std::streampos               buffer_start_pos,
                        std::vector<std::byte>&      buffer_data,
                        std::vector<std::byte>&      compressed_temp_data,
                        const etl_file::header_data& file_header_,
                        event_observer&              callbacks)
{
    file_stream.seekg(buffer_start_pos);
    file_stream.read(reinterpret_cast<char*>(buffer_data.data()), parser::wmi_buffer_header_view::static_size);

    const auto read_header_bytes = file_stream.tellg() - buffer_start_pos;
    if(read_header_bytes < static_cast<std::streamoff>(parser::wmi_buffer_header_view::static_size))
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: insufficient size for buffer header. Expected {} but read only {}.",
            parser::wmi_buffer_header_view::static_size,
            read_header_bytes));
    }
    const auto header_buffer = std::span(buffer_data).subspan(0, parser::wmi_buffer_header_view::static_size);

    const auto header = parser::wmi_buffer_header_view(header_buffer);

    callbacks.handle_buffer(file_header_, header);

    if(header.wnode().saved_offset() > buffer_data.size())
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: buffer offset ({}) exceeds maximum buffer size ({}).",
            header.wnode().saved_offset(),
            buffer_data.size()));
    }
    const auto payload_size = header.wnode().saved_offset() - parser::wmi_buffer_header_view::static_size;

    assert(header.wnode().buffer_size() >= parser::wmi_buffer_header_view::static_size); // has already been checked when initially reading the buffer headers
    const auto remaining_buffer_size = header.wnode().buffer_size() - parser::wmi_buffer_header_view::static_size;

    // NOTE: not sure where we need to additionally test for
    //         file_header_.log_file_mode.test(parser::log_file_mode::compressed_mode) &&
    //         !file_header_.log_file_mode.test(parser::log_file_mode::file_mode_circular)
    const auto is_compressed = header.buffer_flag().test(parser::etw_buffer_flag::compressed);

    if(!is_compressed)
    {
        if(header.wnode().saved_offset() > header.wnode().buffer_size())
        {
            throw std::runtime_error(std::format(
                "Invalid ETL file: buffer offset ({}) exceeds buffer size ({}).",
                header.wnode().saved_offset(),
                header.wnode().buffer_size()));
        }

        // Read the remaining data right into the final buffer.
        file_stream.read(reinterpret_cast<char*>(buffer_data.data() + parser::wmi_buffer_header_view::static_size), remaining_buffer_size);
    }
    else
    {
        // Read the remaining (compressed) data into a temporary buffer.
        compressed_temp_data.resize(remaining_buffer_size);

        file_stream.read(reinterpret_cast<char*>(compressed_temp_data.data()), remaining_buffer_size);
    }

    const auto read_remaining_bytes = file_stream.tellg() - buffer_start_pos - parser::wmi_buffer_header_view::static_size;

    if(read_remaining_bytes < remaining_buffer_size)
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: insufficient size for buffer payload. Expected {} but read only {}.",
            remaining_buffer_size,
            read_remaining_bytes));
    }

    const auto payload_buffer = std::span(buffer_data).subspan(parser::wmi_buffer_header_view::static_size, payload_size);

    if(is_compressed)
    {
        const auto decompressed_size = common::ms_xca_decompress(std::span(compressed_temp_data).subspan(0, remaining_buffer_size),
                                                                 payload_buffer,
                                                                 file_header_.compression_format);

        if(decompressed_size != payload_size)
        {
            throw std::runtime_error(std::format(
                "Invalid ETL file: uncompressed size does not patch payload size. Expected {} but got {}.",
                payload_size,
                decompressed_size));
        }
    }

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
template<typename TraceHeaderViewType>
std::size_t process_type_1_trace(std::span<const std::byte>   payload_buffer,
                                 const etl_file::header_data& file_header,
                                 event_observer&              callbacks)
{
    const auto trace_header = TraceHeaderViewType(payload_buffer.subspan(0, TraceHeaderViewType::static_size));

    assert(trace_header.packet().size() >= TraceHeaderViewType::static_size);
    const auto user_data_size = trace_header.packet().size() - TraceHeaderViewType::static_size;

    const auto user_data_buffer = payload_buffer.subspan(TraceHeaderViewType::static_size, user_data_size);

    callbacks.handle(file_header, trace_header, user_data_buffer);

    return static_cast<std::size_t>(trace_header.packet().size());
}

std::size_t process_perfinfo_trace(std::span<const std::byte>   payload_buffer,
                                   const etl_file::header_data& file_header,
                                   event_observer&              callbacks)
{
    const auto extended_size = parser::perfinfo_trace_header_view::peak_extended_size(payload_buffer.subspan(0, 4));

    const auto header_size  = parser::perfinfo_trace_header_view::static_size + extended_size;
    auto       trace_header = parser::perfinfo_trace_header_view(payload_buffer.subspan(0, header_size));

    assert(trace_header.packet().size() >= header_size);

    const auto user_data_size = trace_header.packet().size() - header_size;

    const auto user_data_buffer = payload_buffer.subspan(header_size, user_data_size);

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
    const auto trace_header = TraceHeaderViewType(payload_buffer.subspan(0, TraceHeaderViewType::static_size));

    assert(trace_header.size() >= TraceHeaderViewType::static_size);
    const auto user_data_size = trace_header.size() - TraceHeaderViewType::static_size;

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
    const auto trace_header = parser::event_header_trace_header_view(payload_buffer.subspan(0, parser::event_header_trace_header_view::static_size));

    [[maybe_unused]] const auto is_extended = (trace_header.flags() & static_cast<std::underlying_type_t<parser::event_header_flag>>(parser::event_header_flag::extended_info)) != 0;

    assert(!is_extended); // not yet supported

    assert(trace_header.size() >= parser::event_header_trace_header_view::static_size);
    const auto user_data_size = trace_header.size() - parser::event_header_trace_header_view::static_size;

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
        read_bytes = process_perfinfo_trace(payload_buffer, file_header, callbacks);
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
    std::array<std::byte, parser::wmi_buffer_header_view::static_size> file_buffer_header_data;

    file_stream_.read(reinterpret_cast<char*>(file_buffer_header_data.data()), file_buffer_header_data.size());

    const auto read_header_bytes = file_stream_.tellg() - std::streampos(0);
    if(read_header_bytes < static_cast<std::streamoff>(parser::wmi_buffer_header_view::static_size))
    {
        throw std::runtime_error("Invalid ETL file: insufficient size for buffer header");
    }

    const auto buffer_header = parser::wmi_buffer_header_view(std::span(file_buffer_header_data));

    if(buffer_header.buffer_type() != parser::etw_buffer_type::header)
    {
        throw std::runtime_error("Invalid ETL file: invalid buffer header type");
    }

    if(buffer_header.buffer_flag().test(parser::etw_buffer_flag::compressed))
    {
        throw std::runtime_error("Invalid ETL file: header buffer should not be compressed");
    }

    const auto min_buffer_size = parser::wmi_buffer_header_view::static_size +
                                 parser::system_trace_header_view::static_size;

    const auto buffer_size = buffer_header.wnode().buffer_size();

    if(buffer_header.wnode().buffer_size() < min_buffer_size)
    {
        throw std::runtime_error(std::format(
            "Invalid ETL file: invalid header buffer size. Is {} but should be at least {}.",
            buffer_size,
            min_buffer_size));
    }

    std::vector<std::byte> file_buffer_data(buffer_size - parser::wmi_buffer_header_view::static_size);

    file_stream_.read(reinterpret_cast<char*>(file_buffer_data.data()), file_buffer_data.size());

    const auto read_buffer_bytes = file_stream_.tellg() - std::streampos(0);
    if(read_buffer_bytes < static_cast<std::streamoff>(min_buffer_size))
    {
        throw std::runtime_error("Invalid ETL file: insufficient size for buffer");
    }

    const auto buffer_data = std::span(file_buffer_data);

    const auto marker = parser::generic_trace_marker_view(buffer_data);
    if(!marker.is_trace_header() || !marker.is_trace_header_event_trace() || marker.is_trace_message())
    {
        throw std::runtime_error("Invalid ETL file: invalid trace marker");
    }
    if(marker.header_type() != parser::trace_header_type::system32 &&
       marker.header_type() != parser::trace_header_type::system64)
    {
        throw std::runtime_error("Invalid ETL file: invalid trace header type");
    }

    const auto system_trace_header = parser::system_trace_header_view(buffer_data);

    // the first record needs to be a event-trace-header
    if(system_trace_header.packet().group() != parser::event_trace_group::header ||
       system_trace_header.packet().type() != 0 ||
       system_trace_header.version() != 2)
    {
        throw std::runtime_error("Invalid ETL file: invalid initial event trace header record.");
    }

    const auto header_event = parser::event_trace_v2_header_event_view(buffer_data.subspan(
        parser::system_trace_header_view::static_size));

    // The compression format is hidden in the state of the first buffer
    const auto compression_format = header_event.log_file_mode().test(parser::log_file_mode::compressed_mode) ?
                                        static_cast<common::ms_xca_compression_format>(buffer_header.wnode().state() & 0xFFFF) :
                                        common::ms_xca_compression_format::none;

    header_ = etl_file::header_data{
        .start_time           = common::from_nt_timestamp(common::nt_duration(header_event.start_time())),
        .end_time             = common::from_nt_timestamp(common::nt_duration(header_event.end_time())),
        .start_time_qpc_ticks = system_trace_header.system_time(),
        .qpc_frequency        = header_event.perf_freq(),
        .pointer_size         = header_event.pointer_size(),
        .number_of_processors = header_event.number_of_processors(),
        .number_of_buffers    = header_event.buffers_written(),
        .buffer_size          = header_event.buffer_size(),
        .log_file_mode        = header_event.log_file_mode(),
        .compression_format   = compression_format
        // TODO: extract more (all?) relevant data
    };

    file_stream_.seekg(0);
}

void etl_file::close()
{
    file_stream_.close();
}

void etl_file::process(event_observer&                   callbacks,
                       const common::progress_listener*  progress_listener,
                       const common::cancellation_token* cancellation_token)
{
    std::vector<processor_data> per_processor_data{header_.number_of_processors};

    common::progress_reporter progress(progress_listener,
                                       header_.number_of_buffers * header_.buffer_size,
                                       "Processing events");

    {
        std::array<std::byte, parser::wmi_buffer_header_view::static_size> header_buffer_data;
        for(std::size_t buffer_index = 0; buffer_index < header_.number_of_buffers; ++buffer_index)
        {
            if(cancellation_token && cancellation_token->is_canceled()) return;

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

            const auto buffer_size = buffer_header.wnode().buffer_size();

            if(buffer_size > header_.buffer_size)
            {
                throw std::runtime_error(std::format(
                    "Unsupported ETL file: buffer size ({}) has buffer size {} but size according to header event is {}",
                    buffer_index,
                    buffer_size,
                    header_.buffer_size));
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
    std::vector<std::byte>                        compressed_temp_data;
    for(std::size_t processor_index = 0; processor_index < per_processor_data.size(); ++processor_index)
    {
        if(cancellation_token && cancellation_token->is_canceled()) return;

        auto& processor_data    = per_processor_data[processor_index];
        auto& remaining_buffers = processor_data.remaining_buffers;

        if(remaining_buffers.empty()) continue;

        std::ranges::sort(remaining_buffers, [](const processor_data::file_buffer_info& lhs, const processor_data::file_buffer_info& rhs)
                          { return lhs.sequence_number > rhs.sequence_number; });

        processor_data.current_buffer_data.resize(header_.buffer_size);

        processor_data.current_buffer_info = read_buffer(file_stream_,
                                                         remaining_buffers.back().start_pos,
                                                         processor_data.current_buffer_data,
                                                         compressed_temp_data,
                                                         header_,
                                                         callbacks);

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
        if(cancellation_token && cancellation_token->is_canceled()) return;

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

            progress.progress(trace_read_bytes);

            // Check whether this was the last event in the current processors buffer
            const auto buffer_exhausted = buffer_info.current_payload_offset >= buffer_info.payload_buffer.size();
            if(buffer_exhausted)
            {
                progress.progress(header_.buffer_size - buffer_info.current_payload_offset);

                auto& remaining_buffers = processor_data.remaining_buffers;

                // If there are no remaining buffers for this processor, stop extracting events for it.
                if(remaining_buffers.empty()) break;

                // Read the next buffer for this processor.
                // Keep in mind, that `remaining_buffers` is sorted.
                buffer_info = read_buffer(file_stream_,
                                          remaining_buffers.back().start_pos,
                                          processor_data.current_buffer_data,
                                          compressed_temp_data,
                                          header_,
                                          callbacks);

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

    progress.finish();
}

const etl_file::header_data& etl_file::header() const
{
    return header_;
}
