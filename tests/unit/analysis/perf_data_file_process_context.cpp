#include <gtest/gtest.h>

#include <snail/analysis/detail/perf_data_file_process_context.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>

#include <snail/common/cast.hpp>

#include "helpers.hpp"

using namespace snail;
using namespace snail::analysis::detail;

using namespace snail::detail::tests;

namespace {

static const auto event_attributes = perf_data::parser::event_attributes{
    // in the following, only sample_format is used.
    .type          = {},
    .sample_format = perf_data::parser::sample_format_flags(295),
    .read_format   = {},
    .flags         = {},
    .precise_ip    = {},
    .name          = {}};

void push_comm_event(perf_data::dispatching_event_observer& observer,
                     std::span<std::byte>                   buffer,
                     std::uint64_t                          time,
                     std::uint32_t                          pid,
                     std::uint32_t                          tid,
                     std::string_view                       comm)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data_size = perf_data::parser::event_header_view::static_size +
                                 8 + comm.size() + 1 +
                                 16;

    set_at(buffer, 0, static_cast<std::uint32_t>(perf_data::parser::event_type::comm));
    set_at(buffer, 6, common::narrow_cast<std::uint16_t>(event_data_size));

    set_at(buffer, perf_data::parser::event_header_view::static_size + 0, pid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 4, tid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 8, comm);

    set_at(buffer, event_data_size - 16, pid);
    set_at(buffer, event_data_size - 12, tid);
    set_at(buffer, event_data_size - 8, time);

    const auto event_data = buffer.subspan(0, event_data_size);

    const auto event_header = perf_data::parser::event_header_view(event_data, std::endian::little);

    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).pid() == pid);
    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).tid() == tid);
    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).comm() == comm);

    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).sample_id().pid == pid);
    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).sample_id().tid == tid);
    assert(perf_data::parser::comm_event_view(event_attributes, event_data, std::endian::little).sample_id().time == time);

    observer.handle(event_header, event_attributes, event_data, std::endian::little);
}

void push_fork_event(perf_data::dispatching_event_observer& observer,
                     std::span<std::byte>                   buffer,
                     std::uint64_t                          time,
                     std::uint32_t                          pid,
                     std::uint32_t                          tid)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data_size = perf_data::parser::event_header_view::static_size +
                                 perf_data::parser::fork_event_view::static_size +
                                 16;

    set_at(buffer, 0, static_cast<std::uint32_t>(perf_data::parser::event_type::fork));
    set_at(buffer, 6, common::narrow_cast<std::uint16_t>(event_data_size));

    set_at(buffer, perf_data::parser::event_header_view::static_size + 0, pid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 8, tid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 16, time);

    set_at(buffer, event_data_size - 16, pid);
    set_at(buffer, event_data_size - 12, tid);
    set_at(buffer, event_data_size - 8, time);

    const auto event_data = buffer.subspan(0, event_data_size);

    const auto event_header = perf_data::parser::event_header_view(event_data, std::endian::little);

    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).pid() == pid);
    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).tid() == tid);
    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).time() == time);

    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).sample_id().pid == pid);
    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).sample_id().tid == tid);
    assert(perf_data::parser::fork_event_view(event_attributes, event_data, std::endian::little).sample_id().time == time);

    observer.handle(event_header, event_attributes, event_data, std::endian::little);
}

void push_mmap2_event(perf_data::dispatching_event_observer& observer,
                      std::span<std::byte>                   buffer,
                      std::uint64_t                          time,
                      std::uint32_t                          pid,
                      std::uint32_t                          tid,
                      std::uint64_t                          addr,
                      std::uint64_t                          len,
                      std::uint64_t                          pgoff,
                      std::string_view                       filename,
                      std::optional<perf_data::build_id>     build_id)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data_size = perf_data::parser::event_header_view::static_size +
                                 64 + filename.size() + 1 +
                                 16;

    set_at(buffer, 0, static_cast<std::uint32_t>(perf_data::parser::event_type::mmap2));
    if(build_id) set_at(buffer, 4, std::to_underlying(perf_data::parser::header_misc_mask::mmap_build_id));
    set_at(buffer, 6, common::narrow_cast<std::uint16_t>(event_data_size));

    set_at(buffer, perf_data::parser::event_header_view::static_size + 0, pid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 4, tid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 8, addr);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 16, len);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 24, pgoff);
    if(build_id)
    {
        set_at(buffer, perf_data::parser::event_header_view::static_size + 32, build_id->size_);
        std::ranges::copy(std::span(build_id->buffer_).subspan(0, build_id->size_), buffer.begin() + perf_data::parser::event_header_view::static_size + 36);
    }
    set_at(buffer, perf_data::parser::event_header_view::static_size + 64, filename);

    set_at(buffer, event_data_size - 16, pid);
    set_at(buffer, event_data_size - 12, tid);
    set_at(buffer, event_data_size - 8, time);

    const auto event_data = buffer.subspan(0, event_data_size);

    const auto event_header = perf_data::parser::event_header_view(event_data, std::endian::little);

    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).pid() == pid);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).tid() == tid);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).addr() == addr);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).len() == len);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).pgoff() == pgoff);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).filename() == filename);

    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).sample_id().pid == pid);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).sample_id().tid == tid);
    assert(perf_data::parser::mmap2_event_view(event_attributes, event_data, std::endian::little).sample_id().time == time);

    observer.handle(event_header, event_attributes, event_data, std::endian::little);
}

void push_sample_event(perf_data::dispatching_event_observer& observer,
                       std::span<std::byte>                   buffer,
                       std::uint64_t                          time,
                       std::uint32_t                          pid,
                       std::uint32_t                          tid,
                       std::uint64_t                          ip,
                       std::vector<std::uint64_t>             ips)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data_size = perf_data::parser::event_header_view::static_size +
                                 40 + ips.size() * 8;

    set_at(buffer, 0, static_cast<std::uint32_t>(perf_data::parser::event_type::sample));
    set_at(buffer, 6, common::narrow_cast<std::uint16_t>(event_data_size));

    set_at(buffer, perf_data::parser::event_header_view::static_size + 0, ip);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 8, pid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 12, tid);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 16, time);
    set_at(buffer, perf_data::parser::event_header_view::static_size + 24, std::uint64_t(1)); // period
    set_at(buffer, perf_data::parser::event_header_view::static_size + 32, common::narrow_cast<std::uint64_t>(ips.size()));
    for(std::size_t i = 0; i < ips.size(); ++i)
    {
        set_at(buffer, perf_data::parser::event_header_view::static_size + 40 + i * 8, ips[i]);
    }

    const auto event_data = buffer.subspan(0, event_data_size);

    const auto event_header = perf_data::parser::event_header_view(event_data, std::endian::little);

    observer.handle(event_header, event_attributes, event_data, std::endian::little);
}

} // namespace

TEST(PerfDataFileProcessContext, Processes)
{
    perf_data_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // The initial process comm event is usually perf-exec itself.
    push_comm_event(context.observer(), writable_bytes_buffer,
                    0, 123, 123, "perf-exec");

    // Then it gets renamed to the actual process we wanted to profile
    push_comm_event(context.observer(), writable_bytes_buffer,
                    10, 123, 123, "proc-a");

    // fork a new process from it
    push_comm_event(context.observer(), writable_bytes_buffer,
                    15, 456, 456, "proc-b");
    push_fork_event(context.observer(), writable_bytes_buffer,
                    15, 456, 456);

    // Then create a new thread for proc-b
    push_fork_event(context.observer(), writable_bytes_buffer,
                    20, 456, 111);

    // And another one for proc-a...
    push_fork_event(context.observer(), writable_bytes_buffer,
                    25, 456, 222);
    // Which gets a name...
    push_comm_event(context.observer(), writable_bytes_buffer,
                    25, 456, 222, "thread-222");

    context.finish();

    const auto& processes = context.get_processes().all_entries();
    EXPECT_EQ(processes.size(), 2);

    const auto& processes_123 = processes.at(123);
    EXPECT_EQ(processes_123.size(), 2);
    EXPECT_EQ(processes_123[0].id, 123);
    EXPECT_EQ(processes_123[0].timestamp, 0);
    EXPECT_EQ(processes_123[0].payload.name, "perf-exec");
    // EXPECT_EQ(processes_123[0].payload.end_time, 10);
    EXPECT_EQ(processes_123[1].id, 123);
    EXPECT_EQ(processes_123[1].timestamp, 10);
    EXPECT_EQ(processes_123[1].payload.name, "proc-a");
    EXPECT_EQ(processes_123[1].payload.end_time, std::nullopt);

    const auto& processes_456 = processes.at(456);
    EXPECT_EQ(processes_456.size(), 1);
    EXPECT_EQ(processes_456[0].id, 456);
    EXPECT_EQ(processes_456[0].timestamp, 15);
    EXPECT_EQ(processes_456[0].payload.name, "proc-b");
    EXPECT_EQ(processes_456[0].payload.end_time, std::nullopt);

    const auto& threads = context.get_threads().all_entries();
    EXPECT_EQ(threads.size(), 4);

    const auto& threads_123 = threads.at(123);
    EXPECT_EQ(threads_123.size(), 2);
    EXPECT_EQ(threads_123[0].id, 123);
    EXPECT_EQ(threads_123[0].timestamp, 0);
    EXPECT_EQ(threads_123[0].payload.name, "perf-exec");
    // EXPECT_EQ(threads_123[0].payload.end_time, 10);
    EXPECT_EQ(threads_123[1].id, 123);
    EXPECT_EQ(threads_123[1].timestamp, 10);
    EXPECT_EQ(threads_123[1].payload.name, "proc-a");
    EXPECT_EQ(threads_123[1].payload.end_time, std::nullopt);

    const auto& threads_456 = threads.at(456);
    EXPECT_EQ(threads_456.size(), 1);
    EXPECT_EQ(threads_456[0].id, 456);
    EXPECT_EQ(threads_456[0].timestamp, 15);
    EXPECT_EQ(threads_456[0].payload.name, "proc-b");
    EXPECT_EQ(threads_456[0].payload.end_time, std::nullopt);

    const auto& threads_111 = threads.at(111);
    EXPECT_EQ(threads_111.size(), 1);
    EXPECT_EQ(threads_111[0].id, 111);
    EXPECT_EQ(threads_111[0].timestamp, 20);
    EXPECT_EQ(threads_111[0].payload.name, std::nullopt);
    EXPECT_EQ(threads_111[0].payload.end_time, std::nullopt);

    const auto& threads_222 = threads.at(222);
    EXPECT_EQ(threads_222.size(), 1);
    EXPECT_EQ(threads_222[0].id, 222);
    EXPECT_EQ(threads_222[0].timestamp, 25);
    EXPECT_EQ(threads_222[0].payload.name, "thread-222");
    EXPECT_EQ(threads_222[0].payload.end_time, std::nullopt);

    using threads_per_proc_set = std::set<std::pair<perf_data_file_process_context::thread_id_t, perf_data_file_process_context::timestamp_t>>;

    EXPECT_EQ(context.get_process_threads(123), (threads_per_proc_set{
                                                    {123, 0 },
                                                    {123, 10}
    }));

    EXPECT_EQ(context.get_process_threads(456), (threads_per_proc_set{
                                                    {456, 15},
                                                    {111, 20},
                                                    {222, 25}
    }));
}

TEST(PerfDataFileProcessContext, Images)
{
    perf_data_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_mmap2_event(context.observer(), writable_bytes_buffer,
                     10, 123, 123, 0xAABB, 1000, 100, "a.so", std::nullopt);

    const auto id_b = perf_data::build_id{
        .size_   = 20,
        .buffer_ = {
                    std::byte(0xef), std::byte(0xdd), std::byte(0x0b), std::byte(0x5e), std::byte(0x69), std::byte(0xb0), std::byte(0x74), std::byte(0x2f), std::byte(0xa5), std::byte(0xe5),
                    std::byte(0xba), std::byte(0xd0), std::byte(0x77), std::byte(0x1d), std::byte(0xf4), std::byte(0xd1), std::byte(0xdf), std::byte(0x24), std::byte(0x59), std::byte(0xd1)}
    };
    push_mmap2_event(context.observer(), writable_bytes_buffer,
                     20, 456, 111, 0xCCDD, 2000, 200, "b.so", id_b);

    context.finish();

    const auto& modules_123 = context.get_modules(123);
    EXPECT_EQ(modules_123.all_modules().size(), 1);

    const auto& module_a = modules_123.all_modules().at(0);
    EXPECT_EQ(module_a.base, 0xAABB);
    EXPECT_EQ(module_a.size, 1000);
    EXPECT_EQ(module_a.payload.page_offset, 100);
    EXPECT_EQ(module_a.payload.filename, "a.so");
    EXPECT_EQ(module_a.payload.build_id, std::nullopt);

    const auto& modules_456 = context.get_modules(456);
    EXPECT_EQ(modules_456.all_modules().size(), 1);

    const auto& module_b = modules_456.all_modules().at(0);
    EXPECT_EQ(module_b.base, 0xCCDD);
    EXPECT_EQ(module_b.size, 2000);
    EXPECT_EQ(module_b.payload.page_offset, 200);
    EXPECT_EQ(module_b.payload.filename, "b.so");
    EXPECT_EQ(module_b.payload.build_id, id_b);
}

TEST(PerfDataFileProcessContext, Samples)
{
    perf_data_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_sample_event(context.observer(), writable_bytes_buffer,
                      20, 123, 123, 0xAA11, {0xAAA1, 0xAAA2, 0xAAA3});

    push_sample_event(context.observer(), writable_bytes_buffer,
                      25, 123, 222, 0xAA22, {0xAAB1, 0xAAB2});

    push_sample_event(context.observer(), writable_bytes_buffer,
                      30, 123, 222, 0xAA33, {0xAAA1, 0xAAA2, 0xAAA3});

    push_sample_event(context.observer(), writable_bytes_buffer,
                      40, 456, 456, 0xBB11, {0xBBA1, 0xBBA2});

    context.finish();

    EXPECT_EQ(context.get_samples_per_process().size(), 2);

    EXPECT_TRUE(context.get_samples_per_process().contains(123));
    const auto& samples_123 = context.get_samples_per_process().at(123);
    EXPECT_EQ(samples_123.first_sample_time, 20);
    EXPECT_EQ(samples_123.last_sample_time, 30);
    EXPECT_EQ(samples_123.samples.size(), 3);

    EXPECT_EQ(samples_123.samples[0].thread_id, 123);
    EXPECT_EQ(samples_123.samples[0].timestamp, 20);
    EXPECT_EQ(samples_123.samples[0].instruction_pointer, 0xAA11);
    EXPECT_EQ(samples_123.samples[0].stack_index, 0);

    EXPECT_EQ(samples_123.samples[1].thread_id, 222);
    EXPECT_EQ(samples_123.samples[1].timestamp, 25);
    EXPECT_EQ(samples_123.samples[1].instruction_pointer, 0xAA22);
    EXPECT_EQ(samples_123.samples[1].stack_index, 1);

    EXPECT_EQ(samples_123.samples[2].thread_id, 222);
    EXPECT_EQ(samples_123.samples[2].timestamp, 30);
    EXPECT_EQ(samples_123.samples[2].instruction_pointer, 0xAA33);
    EXPECT_EQ(samples_123.samples[2].stack_index, 0);

    EXPECT_TRUE(context.get_samples_per_process().contains(456));
    const auto& samples_456 = context.get_samples_per_process().at(456);
    EXPECT_EQ(samples_456.first_sample_time, 40);
    EXPECT_EQ(samples_456.last_sample_time, 40);
    EXPECT_EQ(samples_456.samples.size(), 1);

    EXPECT_EQ(samples_456.samples[0].thread_id, 456);
    EXPECT_EQ(samples_456.samples[0].timestamp, 40);
    EXPECT_EQ(samples_456.samples[0].instruction_pointer, 0xBB11);
    EXPECT_EQ(samples_456.samples[0].stack_index, 2);

    EXPECT_EQ(context.stack(0), (std::vector<std::uint64_t>{0xAAA1, 0xAAA2, 0xAAA3}));
    EXPECT_EQ(context.stack(1), (std::vector<std::uint64_t>{0xAAB1, 0xAAB2}));
    EXPECT_EQ(context.stack(2), (std::vector<std::uint64_t>{0xBBA1, 0xBBA2}));
}
