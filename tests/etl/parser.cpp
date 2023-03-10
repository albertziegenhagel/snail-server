
#include <array>

#include <gtest/gtest.h>

#include <snail/etl/parser/buffer.hpp>

#include <snail/etl/parser/trace_headers/event_header_trace.hpp>
#include <snail/etl/parser/trace_headers/full_header_trace.hpp>
#include <snail/etl/parser/trace_headers/perfinfo_trace.hpp>
#include <snail/etl/parser/trace_headers/system_trace.hpp>

#include <snail/etl/parser/records/kernel/header.hpp>
#include <snail/etl/parser/records/kernel/image.hpp>
#include <snail/etl/parser/records/kernel/perfinfo.hpp>
#include <snail/etl/parser/records/kernel/process.hpp>
#include <snail/etl/parser/records/kernel/stackwalk.hpp>
#include <snail/etl/parser/records/kernel/thread.hpp>

#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>

using namespace snail;

TEST(EtlParser, WmiBufferHeader)
{
    const std::array<std::uint8_t, 72> buffer = {
        0x00, 0x00, 0x01, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xb8, 0x01, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto buffer_header = etl::parser::wmi_buffer_header_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(buffer_header.wnode().buffer_size(), 65536);
    EXPECT_EQ(buffer_header.wnode().saved_offset(), 440);
    EXPECT_EQ(buffer_header.wnode().current_offset(), 0);
    EXPECT_EQ(buffer_header.wnode().reference_count(), 0);
    EXPECT_EQ(buffer_header.wnode().timestamp(), 0);
    EXPECT_EQ(buffer_header.wnode().sequence_number(), 0);
    EXPECT_EQ(buffer_header.wnode().clock(), 0);
    EXPECT_EQ(buffer_header.wnode().client_context().processor_index(), 0);
    EXPECT_EQ(buffer_header.wnode().client_context().logger_id(), 0);
    EXPECT_EQ(buffer_header.wnode().state(), etl::parser::etw_buffer_state::free_);
    EXPECT_EQ(buffer_header.offset(), 440);
    EXPECT_EQ(buffer_header.buffer_flag(), 1);
    EXPECT_EQ(buffer_header.buffer_type(), etl::parser::etw_buffer_type::header);
    EXPECT_EQ(buffer_header.start_time(), 0);
    EXPECT_EQ(buffer_header.start_perf_clock(), 0);
}

TEST(EtlParser, SystemTraceHeader)
{
    const std::array<std::uint8_t, 32> buffer = {
        0x02, 0x00, 0x02, 0xc0, 0x6c, 0x01, 0x00, 0x00, 0x78, 0x47, 0x00, 0x00, 0x44, 0x4c, 0x00, 0x00,
        0x85, 0xcc, 0x05, 0x42, 0xcb, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto trace_header = etl::parser::system_trace_header_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(trace_header.version(), 2);
    EXPECT_EQ(trace_header.header_type(), etl::parser::trace_header_type::system64);
    EXPECT_EQ(trace_header.header_flags(), etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag);
    EXPECT_EQ(trace_header.packet().size(), 364);
    EXPECT_EQ(trace_header.packet().type(), 0);
    EXPECT_EQ(trace_header.packet().group(), etl::parser::event_trace_group::header);
    EXPECT_EQ(trace_header.thread_id(), 18296);
    EXPECT_EQ(trace_header.process_id(), 19524);
    EXPECT_EQ(trace_header.system_time(), 3072009292933);
    EXPECT_EQ(trace_header.kernel_time(), 0);
    EXPECT_EQ(trace_header.user_time(), 0);
}

TEST(EtlParser, PerfInfoTraceHeader)
{
    const std::array<std::uint8_t, 16> buffer = {
        0x02, 0x00, 0x11, 0xc0, 0x20, 0x00, 0x2e, 0x0f, 0x6d, 0x11, 0x06, 0x42, 0xcb, 0x02, 0x00, 0x00};

    const auto trace_header = etl::parser::perfinfo_trace_header_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(trace_header.version(), 2);
    EXPECT_EQ(trace_header.header_type(), etl::parser::trace_header_type::perfinfo64);
    EXPECT_EQ(trace_header.header_flags(), etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag);
    EXPECT_EQ(trace_header.packet().size(), 32);
    EXPECT_EQ(trace_header.packet().type(), 46);
    EXPECT_EQ(trace_header.packet().group(), etl::parser::event_trace_group::perfinfo);
}

TEST(EtlParser, FullHeaderTraceHeader)
{
    const std::array<std::uint8_t, 48> buffer = {
        0x84, 0x01, 0x14, 0xc0, 0x40, 0x00, 0x00, 0x00, 0x20, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x1c, 0x18, 0x06, 0x42, 0xcb, 0x02, 0x00, 0x00, 0xd7, 0x75, 0xe6, 0xb3, 0x54, 0x25, 0x18, 0x4f,
        0x83, 0x0b, 0x27, 0x62, 0x73, 0x25, 0x60, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto trace_header = etl::parser::full_header_trace_header_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(trace_header.size(), 388);
    EXPECT_EQ(trace_header.header_type(), etl::parser::trace_header_type::full_header64);
    EXPECT_EQ(trace_header.header_flags(), etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag);
    EXPECT_EQ(trace_header.trace_class().type(), 64);
    EXPECT_EQ(trace_header.trace_class().level(), 0);
    EXPECT_EQ(trace_header.trace_class().version(), 0);
    EXPECT_EQ(trace_header.thread_id(), 26400);
    EXPECT_EQ(trace_header.process_id(), 0);
    EXPECT_EQ(trace_header.timestamp(), 3072009312284);
    EXPECT_EQ(trace_header.guid().instantiate(), (etl::guid{
                                                     0xb3e675d7, 0x2554, 0x4f18, {0x83, 0x0b, 0x27, 0x62, 0x73, 0x25, 0x60, 0xde}
    }));
    EXPECT_EQ(trace_header.processor_time(), 0);
}

TEST(EtlParser, EventHeaderTraceHeader)
{
    const std::array<std::uint8_t, 80> buffer = {
        0x64, 0x00, 0x13, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xb1, 0x76, 0xf1, 0x43, 0xcb, 0x02, 0x00, 0x00, 0x46, 0x90, 0x5f, 0x9e, 0xc6, 0x43, 0x62, 0x4f,
        0xba, 0x13, 0x7b, 0x19, 0x89, 0x62, 0x53, 0xff, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto trace_header = etl::parser::event_header_trace_header_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(trace_header.size(), 100);
    EXPECT_EQ(trace_header.header_type(), etl::parser::trace_header_type::event_header64);
    EXPECT_EQ(trace_header.header_flags(), etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag);
    EXPECT_EQ(trace_header.flags(), 0);
    EXPECT_EQ(trace_header.event_property(), 0);
    EXPECT_EQ(trace_header.thread_id(), 0);
    EXPECT_EQ(trace_header.process_id(), 0);
    EXPECT_EQ(trace_header.timestamp(), 3072041514673);
    EXPECT_EQ(trace_header.provider_id().instantiate(), (etl::guid{
                                                            0x9e5f9046, 0x43c6, 0x4f62, {0xba, 0x13, 0x7b, 0x19, 0x89, 0x62, 0x53, 0xff}
    }));
    EXPECT_EQ(trace_header.event_descriptor().id(), 6);
    EXPECT_EQ(trace_header.event_descriptor().version(), 0);
    EXPECT_EQ(trace_header.event_descriptor().channel(), 0);
    EXPECT_EQ(trace_header.event_descriptor().level(), 0);
    EXPECT_EQ(trace_header.event_descriptor().opcode(), 0);
    EXPECT_EQ(trace_header.event_descriptor().task(), 0);
    EXPECT_EQ(trace_header.event_descriptor().keyword(), 0);
    EXPECT_EQ(trace_header.processor_time(), 0);
    EXPECT_EQ(trace_header.activity_id().instantiate(), (etl::guid{
                                                            0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    }));
}

TEST(EtlParser, EventTraceV2HeaderEvent)
{
    const std::array<std::uint8_t, 332> buffer = {
        0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x01, 0x05, 0x5d, 0x58, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x3b, 0xae, 0x57, 0x29, 0x8d, 0x1e, 0xd9, 0x01, 0x5a, 0x62, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0xea, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xf8, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0xff, 0xff, 0xff, 0x40, 0x00, 0x74, 0x00,
        0x7a, 0x00, 0x72, 0x00, 0x65, 0x00, 0x73, 0x00, 0x2e, 0x00, 0x64, 0x00, 0x6c, 0x00, 0x6c, 0x00,
        0x2c, 0x00, 0x2d, 0x00, 0x33, 0x00, 0x32, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
        0x00, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x74, 0x00, 0x7a, 0x00, 0x72, 0x00, 0x65, 0x00, 0x73, 0x00, 0x2e, 0x00, 0x64, 0x00,
        0x6c, 0x00, 0x6c, 0x00, 0x2c, 0x00, 0x2d, 0x00, 0x33, 0x00, 0x32, 0x00, 0x31, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xc4, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x8a, 0x05, 0xfc, 0xc1, 0x1b, 0xd9, 0x01,
        0x80, 0x96, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x61, 0xe7, 0x1b, 0x8d, 0x1e, 0xd9, 0x01,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x6f, 0x00,
        0x67, 0x00, 0x67, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x6d, 0x00, 0x75, 0x00,
        0x6c, 0x00, 0x74, 0x00, 0x69, 0x00, 0x70, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x20, 0x00, 0x66, 0x00,
        0x69, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x73, 0x00, 0x5d, 0x00, 0x00, 0x00};

    const auto event = etl::parser::event_trace_v2_header_event_view(std::as_bytes(std::span(buffer)));

    EXPECT_EQ(event.buffer_size(), 65536);
    EXPECT_EQ(event.version(), 83951626);
    EXPECT_EQ(event.provider_version(), 22621);
    EXPECT_EQ(event.number_of_processors(), 8);
    EXPECT_EQ(event.end_time(), 133171255616974395ULL);
    EXPECT_EQ(event.timer_resolution(), 156250);
    EXPECT_EQ(event.max_file_size(), 0);
    EXPECT_EQ(event.log_file_mode(), 65537);
    EXPECT_EQ(event.buffers_written(), 490);
    EXPECT_EQ(event.start_buffers(), 1);
    EXPECT_EQ(event.pointer_size(), 8);
    EXPECT_EQ(event.events_lost(), 0);
    EXPECT_EQ(event.cpu_speed(), 2808);
    EXPECT_EQ(event.logger_name(), 0);
    EXPECT_EQ(event.log_file_name(), 0);
    EXPECT_EQ(event.time_zone_information().bias(), 4294967236);
    EXPECT_EQ(event.time_zone_information().standard_name(), std::u16string(u"@tzres.dll,-322"));
    EXPECT_EQ(event.time_zone_information().standard_date().year(), 0);
    EXPECT_EQ(event.time_zone_information().standard_date().month(), 10);
    EXPECT_EQ(event.time_zone_information().standard_date().day_of_week(), 0);
    EXPECT_EQ(event.time_zone_information().standard_date().day(), 5);
    EXPECT_EQ(event.time_zone_information().standard_date().hour(), 3);
    EXPECT_EQ(event.time_zone_information().standard_date().minute(), 0);
    EXPECT_EQ(event.time_zone_information().standard_date().second(), 0);
    EXPECT_EQ(event.time_zone_information().standard_date().milliseconds(), 0);
    EXPECT_EQ(event.time_zone_information().standard_bias(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_name(), std::u16string(u"@tzres.dll,-321"));
    EXPECT_EQ(event.time_zone_information().daylight_date().year(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_date().month(), 3);
    EXPECT_EQ(event.time_zone_information().daylight_date().day_of_week(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_date().day(), 5);
    EXPECT_EQ(event.time_zone_information().daylight_date().hour(), 2);
    EXPECT_EQ(event.time_zone_information().daylight_date().minute(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_date().second(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_date().milliseconds(), 0);
    EXPECT_EQ(event.time_zone_information().daylight_bias(), -60);
    EXPECT_EQ(event.boot_time(), 133168183955000000ULL);
    EXPECT_EQ(event.perf_freq(), 10000000);
    EXPECT_EQ(event.start_time(), 133171255391510931ULL);
    EXPECT_EQ(event.reserved_flags(), 1);
    EXPECT_EQ(event.buffers_lost(), 0);
    EXPECT_EQ(event.session_name(), std::u16string(u"Relogger"));
    EXPECT_EQ(event.file_name(), std::u16string(u"[multiple files]"));
}

TEST(EtlParser, PerfinfoV2SampledProfileEvent)
{
    const std::array<std::uint8_t, 16> buffer = {
        0x8a, 0x35, 0x01, 0x2e, 0x03, 0xf8, 0xff, 0xff, 0x20, 0x67, 0x00, 0x00, 0x01, 0x00, 0x48, 0x00};

    const auto event = etl::parser::perfinfo_v2_sampled_profile_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.instruction_pointer(), 18446735291273262474ULL);
    EXPECT_EQ(event.thread_id(), 26400);
    EXPECT_EQ(event.count(), 4718593);
}

TEST(EtlParser, StackwalkV2StackEvent)
{
    const std::array<std::uint8_t, 120> buffer = {
        0x39, 0xc8, 0x26, 0x42, 0xcb, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0xc6, 0x31, 0x62, 0x03, 0xf8, 0xff, 0xff, 0x8d, 0xc9, 0x31, 0x62, 0x03, 0xf8, 0xff, 0xff,
        0xf7, 0x8e, 0x30, 0x62, 0x03, 0xf8, 0xff, 0xff, 0x3c, 0xf1, 0x30, 0x62, 0x03, 0xf8, 0xff, 0xff,
        0xfe, 0xe2, 0x30, 0x62, 0x03, 0xf8, 0xff, 0xff, 0xaa, 0x2c, 0x00, 0x3f, 0x03, 0xf8, 0xff, 0xff,
        0x81, 0xd3, 0x25, 0x5c, 0x03, 0xf8, 0xff, 0xff, 0xc1, 0xe2, 0xf4, 0x5b, 0x03, 0xf8, 0xff, 0xff,
        0xf9, 0x2f, 0xf3, 0x5b, 0x03, 0xf8, 0xff, 0xff, 0x36, 0x3f, 0x00, 0x3f, 0x03, 0xf8, 0xff, 0xff,
        0xba, 0x99, 0x0b, 0x2e, 0x03, 0xf8, 0xff, 0xff, 0xc4, 0x91, 0x0b, 0x2e, 0x03, 0xf8, 0xff, 0xff,
        0xfe, 0xd4, 0x22, 0x2e, 0x03, 0xf8, 0xff, 0xff};

    const auto event = etl::parser::stackwalk_v2_stack_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.event_timestamp(), 3072011454521);
    EXPECT_EQ(event.process_id(), 0);
    EXPECT_EQ(event.thread_id(), 0);
    EXPECT_EQ(event.stack_size(), 13);
    EXPECT_EQ(event.stack_address(0), 18446735292148860480ULL);
    EXPECT_EQ(event.stack_address(1), 18446735292148861325ULL);
    EXPECT_EQ(event.stack_address(2), 18446735292148780791ULL);
    EXPECT_EQ(event.stack_address(3), 18446735292148805948ULL);
    EXPECT_EQ(event.stack_address(4), 18446735292148802302ULL);
    EXPECT_EQ(event.stack_address(5), 18446735291558407338ULL);
    EXPECT_EQ(event.stack_address(6), 18446735292047414145ULL);
    EXPECT_EQ(event.stack_address(7), 18446735292044206785ULL);
    EXPECT_EQ(event.stack_address(8), 18446735292044095481ULL);
    EXPECT_EQ(event.stack_address(9), 18446735291558412086ULL);
    EXPECT_EQ(event.stack_address(10), 18446735291273943482ULL);
    EXPECT_EQ(event.stack_address(11), 18446735291273941444ULL);
    EXPECT_EQ(event.stack_address(12), 18446735291275465982ULL);
}

TEST(EtlParser, ImageV2LoadEventView)
{
    const std::array<std::uint8_t, 124> buffer = {
        0x00, 0x00, 0xe0, 0x2d, 0x03, 0xf8, 0xff, 0xff, 0x00, 0x70, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x57, 0xe4, 0xb7, 0x00, 0x93, 0x5a, 0x7b, 0xf5, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
        0x74, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x52, 0x00, 0x6f, 0x00, 0x6f, 0x00, 0x74, 0x00, 0x5c, 0x00,
        0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x33, 0x00, 0x32, 0x00,
        0x5c, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x6f, 0x00, 0x73, 0x00, 0x6b, 0x00, 0x72, 0x00, 0x6e, 0x00,
        0x6c, 0x00, 0x2e, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00, 0x00, 0x00};

    const auto event = etl::parser::image_v2_load_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.image_base(), 18446735291271086080ULL);
    EXPECT_EQ(event.image_size(), 17068032);
    EXPECT_EQ(event.process_id(), 0);
    EXPECT_EQ(event.image_checksum(), 12051543);
    EXPECT_EQ(event.time_date_stamp(), 147);
    EXPECT_EQ(event.signature_level(), 90);
    EXPECT_EQ(event.signature_type(), 62843);
    EXPECT_EQ(event.default_base(), 0);
    EXPECT_EQ(event.file_name(), std::u16string(u"\\SystemRoot\\system32\\ntoskrnl.exe"));
}

TEST(EtlParser, ProcessV4TypeGroup1EventView)
{
    const std::array<std::uint8_t, 179> buffer = {
        0xc0, 0x80, 0x2a, 0x5b, 0x01, 0xbf, 0xff, 0xff, 0x70, 0x25, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x40, 0x3d, 0xc4, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xc0, 0xab, 0xec, 0xd2, 0x0d, 0xd0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00,
        0x57, 0x6d, 0x69, 0x50, 0x72, 0x76, 0x53, 0x45, 0x2e, 0x65, 0x78, 0x65, 0x00, 0x43, 0x00, 0x3a,
        0x00, 0x5c, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, 0x00, 0x44, 0x00, 0x4f, 0x00, 0x57, 0x00, 0x53,
        0x00, 0x5c, 0x00, 0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x33,
        0x00, 0x32, 0x00, 0x5c, 0x00, 0x77, 0x00, 0x62, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x5c, 0x00, 0x77,
        0x00, 0x6d, 0x00, 0x69, 0x00, 0x70, 0x00, 0x72, 0x00, 0x76, 0x00, 0x73, 0x00, 0x65, 0x00, 0x2e,
        0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00, 0x20, 0x00, 0x2d, 0x00, 0x45, 0x00, 0x6d, 0x00, 0x62,
        0x00, 0x65, 0x00, 0x64, 0x00, 0x64, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00};

    const auto event = etl::parser::process_v4_type_group1_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.unique_process_key(), 18446672611278225600ULL);
    EXPECT_EQ(event.process_id(), 9584);
    EXPECT_EQ(event.parent_id(), 1152);
    EXPECT_EQ(event.session_id(), 0);
    EXPECT_EQ(event.exit_status(), 259);
    EXPECT_EQ(event.directory_table_base(), 7587315712);
    EXPECT_EQ(event.flags(), 0);

    EXPECT_TRUE(event.has_sid());
    EXPECT_EQ(event.user_sid_token_user(), (std::array<std::uint64_t, 2>{0xffffd00dd2ecabc0, 0}));
    EXPECT_EQ(event.user_sid().revision(), 1);
    EXPECT_EQ(event.user_sid().sub_authority_count(), 1);
    // EXPECT_EQ(event.user_sid().identifier_authority(), 0);
    // EXPECT_EQ(event.user_sid().sub_authority(), 1);

    EXPECT_EQ(event.image_filename(), std::string("WmiPrvSE.exe"));
    EXPECT_EQ(event.command_line(), std::u16string(u"C:\\WINDOWS\\system32\\wbem\\wmiprvse.exe -Embedding"));
    EXPECT_EQ(event.package_full_name(), std::u16string(u""));
    EXPECT_EQ(event.application_id(), std::u16string(u""));
}

TEST(EtlParser, ThreadV3TypeGroup1EventView)
{
    const std::array<std::uint8_t, 96> buffer = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x45, 0x30, 0x03, 0xf8, 0xff, 0xff,
        0x00, 0x40, 0x45, 0x30, 0x03, 0xf8, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x60, 0xd4, 0x22, 0x2e, 0x03, 0xf8, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x49, 0x00, 0x64, 0x00, 0x6c, 0x00, 0x65, 0x00,
        0x20, 0x00, 0x54, 0x00, 0x68, 0x00, 0x72, 0x00, 0x65, 0x00, 0x61, 0x00, 0x64, 0x00, 0x00, 0x00};

    const auto event = etl::parser::thread_v3_type_group1_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.process_id(), 0);
    EXPECT_EQ(event.thread_id(), 0);
    EXPECT_EQ(event.stack_base(), 18446735291311304704ULL);
    EXPECT_EQ(event.stack_limit(), 18446735291311276032ULL);
    EXPECT_EQ(event.user_stack_base(), 0);
    EXPECT_EQ(event.user_stack_limit(), 0);
    EXPECT_EQ(event.affinity(), 1);
    EXPECT_EQ(event.win32_start_addr(), 18446735291275465824ULL);
    EXPECT_EQ(event.teb_base(), 0);
    EXPECT_EQ(event.sub_process_tag(), 0);
    EXPECT_EQ(event.base_priority(), 0);
    EXPECT_EQ(event.page_priority(), 5);
    EXPECT_EQ(event.io_priority(), 0);
    EXPECT_EQ(event.flags(), 0);
}

TEST(EtlParser, ImageIdV2InfoEventView)
{
    const std::array<std::uint8_t, 50> buffer = {
        0x00, 0x00, 0xe0, 0x2d, 0x03, 0xf8, 0xff, 0xff, 0x00, 0x70, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x6b, 0x00, 0x72, 0x00,
        0x6e, 0x00, 0x6c, 0x00, 0x6d, 0x00, 0x70, 0x00, 0x2e, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
        0x00, 0x00};

    const auto event = etl::parser::image_id_v2_info_event_view(std::as_bytes(std::span(buffer)), 8);

    EXPECT_EQ(event.image_base(), 18446735291271086080ULL);
    EXPECT_EQ(event.image_size(), 17068032);
    EXPECT_EQ(event.process_id(), 0);
    EXPECT_EQ(event.time_date_stamp(), 0);
    EXPECT_EQ(event.original_file_name(), std::u16string(u"ntkrnlmp.exe"));
}
