
#include <array>

#include <gtest/gtest.h>

#include <snail/perf_data/dispatching_event_observer.hpp>
#include <snail/perf_data/parser/records/kernel.hpp>

using namespace snail;

TEST(PerfDataDispatchEventObserver, Dispatch)
{
    perf_data::dispatching_event_observer observer;

    const std::array<std::uint8_t, 48> fork_buffer_data = {
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x3f, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00,
        0x3f, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0xec, 0xf7, 0xab, 0x37, 0xc3, 0x01, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x88, 0xf7, 0xab, 0x37, 0xc3, 0x01, 0x00, 0x00 };

    const std::array<std::uint8_t, 72> sample_buffer_data = {
        0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x48, 0x00, 0x93, 0x80, 0x92, 0x49, 0x93, 0x7f, 0x00, 0x00,
        0x3f, 0x05, 0x00, 0x00, 0x3f, 0x05, 0x00, 0x00, 0x38, 0xb7, 0xf5, 0x37, 0xc3, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x93, 0x80, 0x92, 0x49, 0x93, 0x7f, 0x00, 0x00,
        0xc0, 0x5f, 0x10, 0x83, 0xae, 0x55, 0x00, 0x00 };

    const auto event_attributes = perf_data::parser::event_attributes{
        .sample_format = perf_data::parser::sample_format_flags(295)
    };

    bool fork_event_called = false;
    observer.register_event<perf_data::parser::fork_event_view>(
        [&fork_event_called](const perf_data::parser::event_header_view& /*header*/,
                             const perf_data::parser::fork_event_view& event)
        {
            EXPECT_EQ(event.pid(), 1343);
            EXPECT_EQ(event.ppid(), 1342);

            fork_event_called = true;
        });
    bool sample_event_called = false;
    observer.register_event<perf_data::parser::sample_event>(
        [&sample_event_called](const perf_data::parser::event_header_view& /*header*/,
                               const perf_data::parser::sample_event& event)
        {
            EXPECT_EQ(event.ip, 140270571258003);
            EXPECT_EQ(event.ips, (std::vector<std::uint64_t>{18446744073709551104, 140270571258003, 94208011558848}));

            sample_event_called = true;
        });

    {
        fork_event_called = false;
        sample_event_called = false;
        
        const auto buffer = std::as_bytes(std::span(fork_buffer_data));
        const auto event_header = perf_data::parser::event_header_view(buffer, std::endian::little);
        const auto event_buffer  = buffer.subspan(perf_data::parser::event_header_view::static_size);

        observer.handle(event_header, event_attributes, event_buffer, std::endian::little);

        EXPECT_TRUE(fork_event_called);
        EXPECT_FALSE(sample_event_called);
    }

    {
        fork_event_called = false;
        sample_event_called = false;
        
        const auto buffer = std::as_bytes(std::span(sample_buffer_data));
        const auto event_header = perf_data::parser::event_header_view(buffer, std::endian::little);
        const auto event_buffer  = buffer.subspan(perf_data::parser::event_header_view::static_size);

        observer.handle(event_header, event_attributes, event_buffer, std::endian::little);

        EXPECT_FALSE(fork_event_called);
        EXPECT_TRUE(sample_event_called);
    }
}