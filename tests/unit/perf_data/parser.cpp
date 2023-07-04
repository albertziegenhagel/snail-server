
#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <snail/perf_data/parser/event_attributes.hpp>
#include <snail/perf_data/parser/header.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;

TEST(PerfDataParser, Header)
{
    const std::array<std::uint8_t, 104> buffer = {
        0x50, 0x45, 0x52, 0x46, 0x49, 0x4c, 0x45, 0x32, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xe0, 0x4c, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x3f, 0x31, 0x96, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto header = perf_data::parser::header_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(header.magic(), 0x32454c4946524550);
    EXPECT_EQ(header.size(), 104);
    EXPECT_EQ(header.attributes_size(), 144);
    EXPECT_EQ(header.attributes().offset(), 168);
    EXPECT_EQ(header.attributes().size(), 144);
    EXPECT_EQ(header.data().offset(), 312);
    EXPECT_EQ(header.data().size(), 1068256);
    EXPECT_EQ(header.event_types().offset(), 0);
    EXPECT_EQ(header.event_types().size(), 0);
    EXPECT_EQ(header.additional_features().data(), std::bitset<256>("0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010010110001100010011111111111100"));
}

TEST(PerfDataParser, EventAttributes)
{
    const std::array<std::uint8_t, 144> buffer = {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x37, 0x94, 0x61, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto event_attributes_view = perf_data::parser::event_attributes_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_attributes_view.type(), perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(event_attributes_view.size(), 128);
    EXPECT_EQ(event_attributes_view.config(), 0);
    EXPECT_EQ(event_attributes_view.sample_freq(), 4000);
    EXPECT_EQ(event_attributes_view.sample_format().data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111"));
    EXPECT_EQ(event_attributes_view.read_format().data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100"));
    EXPECT_EQ(event_attributes_view.flags().data(), std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011"));
    EXPECT_EQ(event_attributes_view.precise_ip(), perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_EQ(event_attributes_view.wakeup_events(), 0);
    EXPECT_EQ(event_attributes_view.bp_type(), 0);
    EXPECT_EQ(event_attributes_view.config1(), 0);
    EXPECT_EQ(event_attributes_view.config2(), 0);
    EXPECT_EQ(event_attributes_view.branch_sample_type(), 0);
    EXPECT_EQ(event_attributes_view.sample_regs_user(), 0);
    EXPECT_EQ(event_attributes_view.sample_stack_user(), 0);
    EXPECT_EQ(event_attributes_view.clockid(), 0);
    EXPECT_EQ(event_attributes_view.sample_regs_intr(), 0);
    EXPECT_EQ(event_attributes_view.aux_watermark(), 0);
    EXPECT_EQ(event_attributes_view.sample_max_stack(), 0);
    EXPECT_EQ(event_attributes_view.aux_sample_size(), 0);
    EXPECT_EQ(event_attributes_view.sig_data(), 0);

    const auto event_attributes = event_attributes_view.instantiate();

    EXPECT_EQ(event_attributes.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(event_attributes.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111"));
    EXPECT_EQ(event_attributes.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100"));
    EXPECT_EQ(event_attributes.flags.data(), std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011"));
    EXPECT_EQ(event_attributes.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
}

TEST(PerfDataParser, KernelCommEvent)
{
    const std::array<std::uint8_t, 48> buffer = {
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x70, 0x65, 0x72, 0x66, 0x2d, 0x65, 0x78, 0x65,
        0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto attributes = perf_data::parser::event_attributes{
        // in the following, only sample_format is used.
        .type          = {},
        .sample_format = perf_data::parser::sample_format_flags(295),
        .read_format   = {},
        .flags         = {},
        .precise_ip    = {},
        .name          = {}};

    const auto event_view = perf_data::parser::comm_event_view(attributes, std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::comm_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 40);

    EXPECT_EQ(event_view.pid(), 1342);
    EXPECT_EQ(event_view.tid(), 1342);
    EXPECT_EQ(event_view.comm(), "perf-exec");

    const auto sample_id = event_view.sample_id();
    EXPECT_THAT(sample_id.pid, testing::Optional(0));
    EXPECT_THAT(sample_id.tid, testing::Optional(0));
    EXPECT_THAT(sample_id.time, testing::Optional(0));
    EXPECT_EQ(sample_id.id, std::nullopt);
    EXPECT_EQ(sample_id.stream_id, std::nullopt);
    EXPECT_EQ(sample_id.cpu, std::nullopt);
    EXPECT_EQ(sample_id.res, std::nullopt);
}

TEST(PerfDataParser, KernelExitEvent)
{
    const std::array<std::uint8_t, 48> buffer = {
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3d, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x3d, 0x05, 0x00, 0x00,
        0x08, 0xe7, 0xa0, 0x13, 0xc4, 0x01, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00,
        0x90, 0xe1, 0xa0, 0x13, 0xc4, 0x01, 0x00, 0x00};

    const auto attributes = perf_data::parser::event_attributes{
        // in the following, only sample_format is used.
        .type          = {},
        .sample_format = perf_data::parser::sample_format_flags(295),
        .read_format   = {},
        .flags         = {},
        .precise_ip    = {},
        .name          = {}};

    const auto event_view = perf_data::parser::exit_event_view(attributes, std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::exit_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 40);

    EXPECT_EQ(event_view.pid(), 1342);
    EXPECT_EQ(event_view.ppid(), 1341);
    EXPECT_EQ(event_view.tid(), 1342);
    EXPECT_EQ(event_view.ptid(), 1341);
    EXPECT_EQ(event_view.time(), 1941654529800);

    const auto sample_id = event_view.sample_id();
    EXPECT_THAT(sample_id.pid, testing::Optional(1342));
    EXPECT_THAT(sample_id.tid, testing::Optional(1342));
    EXPECT_THAT(sample_id.time, testing::Optional(1941654528400));
    EXPECT_EQ(sample_id.id, std::nullopt);
    EXPECT_EQ(sample_id.stream_id, std::nullopt);
    EXPECT_EQ(sample_id.cpu, std::nullopt);
    EXPECT_EQ(sample_id.res, std::nullopt);
}

TEST(PerfDataParser, KernelForkEvent)
{
    const std::array<std::uint8_t, 48> buffer = {
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x3f, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x3f, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00,
        0xec, 0xf7, 0xab, 0x37, 0xc3, 0x01, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00,
        0x88, 0xf7, 0xab, 0x37, 0xc3, 0x01, 0x00, 0x00};

    const auto attributes = perf_data::parser::event_attributes{
        // in the following, only sample_format is used.
        .type          = {},
        .sample_format = perf_data::parser::sample_format_flags(295),
        .read_format   = {},
        .flags         = {},
        .precise_ip    = {},
        .name          = {}};

    const auto event_view = perf_data::parser::fork_event_view(attributes, std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::fork_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 40);

    EXPECT_EQ(event_view.pid(), 1343);
    EXPECT_EQ(event_view.ppid(), 1342);
    EXPECT_EQ(event_view.tid(), 1343);
    EXPECT_EQ(event_view.ptid(), 1342);
    EXPECT_EQ(event_view.time(), 1937964267500);

    const auto sample_id = event_view.sample_id();
    EXPECT_THAT(sample_id.pid, testing::Optional(1342));
    EXPECT_THAT(sample_id.tid, testing::Optional(1342));
    EXPECT_THAT(sample_id.time, testing::Optional(1937964267400));
    EXPECT_EQ(sample_id.id, std::nullopt);
    EXPECT_EQ(sample_id.stream_id, std::nullopt);
    EXPECT_EQ(sample_id.cpu, std::nullopt);
    EXPECT_EQ(sample_id.res, std::nullopt);
}

TEST(PerfDataParser, KernelMmap2Event)
{
    const std::array<std::uint8_t, 136> buffer = {
        0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x00, 0x50, 0x0e, 0xce, 0x94, 0x7f, 0x00, 0x00,
        0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6e, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x34, 0xfe, 0x78, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x2f, 0x75, 0x73, 0x72, 0x2f, 0x6c, 0x69, 0x62, 0x36, 0x34, 0x2f, 0x6f, 0x70, 0x65, 0x6e, 0x6d,
        0x70, 0x69, 0x2f, 0x6c, 0x69, 0x62, 0x2f, 0x6f, 0x70, 0x65, 0x6e, 0x6d, 0x70, 0x69, 0x2f, 0x6d,
        0x63, 0x61, 0x5f, 0x62, 0x74, 0x6c, 0x5f, 0x76, 0x61, 0x64, 0x65, 0x72, 0x2e, 0x73, 0x6f, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0xfc, 0xb3, 0x56, 0x4d, 0xc3, 0x01, 0x00, 0x00};

    const auto attributes = perf_data::parser::event_attributes{
        // in the following, only sample_format is used.
        .type          = {},
        .sample_format = perf_data::parser::sample_format_flags(295),
        .read_format   = {},
        .flags         = {},
        .precise_ip    = {},
        .name          = {}};

    const auto event_view = perf_data::parser::mmap2_event_view(attributes, std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::mmap2_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 128);

    EXPECT_EQ(event_view.pid(), 1342);
    EXPECT_EQ(event_view.tid(), 1342);

    EXPECT_EQ(event_view.addr(), 140277088931840);
    EXPECT_EQ(event_view.len(), 20480);
    EXPECT_EQ(event_view.pgoff(), 8192);

    EXPECT_FALSE(event_view.has_build_id());
    EXPECT_EQ(event_view.maj(), 8);
    EXPECT_EQ(event_view.min(), 32);
    EXPECT_EQ(event_view.ino(), 24942);
    EXPECT_EQ(event_view.ino_generation(), 242810420);

    EXPECT_EQ(event_view.prot(), 5);
    EXPECT_EQ(event_view.flags(), 2);
    EXPECT_EQ(event_view.filename(), "/usr/lib64/openmpi/lib/openmpi/mca_btl_vader.so");

    const auto sample_id = event_view.sample_id();
    EXPECT_THAT(sample_id.pid, testing::Optional(1342));
    EXPECT_THAT(sample_id.tid, testing::Optional(1342));
    EXPECT_THAT(sample_id.time, testing::Optional(1938327778300));
    EXPECT_EQ(sample_id.id, std::nullopt);
    EXPECT_EQ(sample_id.stream_id, std::nullopt);
    EXPECT_EQ(sample_id.cpu, std::nullopt);
    EXPECT_EQ(sample_id.res, std::nullopt);
}

TEST(PerfDataParser, KernelSampleEvent)
{
    const std::array<std::uint8_t, 72> buffer = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x93, 0x80, 0x92, 0x49, 0x93, 0x7f, 0x00, 0x00, 0x3f, 0x05, 0x00, 0x00, 0x3f, 0x05, 0x00, 0x00,
        0x38, 0xb7, 0xf5, 0x37, 0xc3, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x93, 0x80, 0x92, 0x49, 0x93, 0x7f, 0x00, 0x00, 0xc0, 0x5f, 0x10, 0x83, 0xae, 0x55, 0x00, 0x00};

    const auto attributes = perf_data::parser::event_attributes{
        // in the following, only sample_format is used.
        .type          = {},
        .sample_format = perf_data::parser::sample_format_flags(295),
        .read_format   = {},
        .flags         = {},
        .precise_ip    = {},
        .name          = {}};

    const auto event = perf_data::parser::parse_event<perf_data::parser::sample_event>(attributes, std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event.id, std::nullopt);
    EXPECT_EQ(event.ip, 140270571258003);
    EXPECT_EQ(event.pid, 1343);
    EXPECT_EQ(event.tid, 1343);
    EXPECT_EQ(event.time, 1937969100600);
    EXPECT_EQ(event.addr, std::nullopt);
    EXPECT_EQ(event.stream_id, std::nullopt);
    EXPECT_EQ(event.cpu, std::nullopt);
    EXPECT_EQ(event.res, std::nullopt);
    EXPECT_EQ(event.period, 1);
    EXPECT_EQ(event.ips, (std::vector<std::uint64_t>{18446744073709551104ULL, 140270571258003ULL, 94208011558848ULL}));
    EXPECT_EQ(event.data, std::nullopt);
}

TEST(PerfDataParser, PerfIdIndexEvent)
{
    const std::array<std::uint8_t, 272> buffer = {
        0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x01,
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto event_view = perf_data::parser::id_index_event_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::id_index_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 264);

    EXPECT_EQ(event_view.nr(), 8);
    EXPECT_EQ(event_view.entry(0).id(), 461);
    EXPECT_EQ(event_view.entry(0).idx(), 0);
    EXPECT_EQ(event_view.entry(0).cpu(), 0);
    EXPECT_EQ(event_view.entry(0).tid(), 1342);
    EXPECT_EQ(event_view.entry(1).id(), 462);
    EXPECT_EQ(event_view.entry(1).idx(), 1);
    EXPECT_EQ(event_view.entry(1).cpu(), 1);
    EXPECT_EQ(event_view.entry(1).tid(), 1342);
    EXPECT_EQ(event_view.entry(2).id(), 463);
    EXPECT_EQ(event_view.entry(2).idx(), 2);
    EXPECT_EQ(event_view.entry(2).cpu(), 2);
    EXPECT_EQ(event_view.entry(2).tid(), 1342);
    EXPECT_EQ(event_view.entry(3).id(), 464);
    EXPECT_EQ(event_view.entry(3).idx(), 3);
    EXPECT_EQ(event_view.entry(3).cpu(), 3);
    EXPECT_EQ(event_view.entry(3).tid(), 1342);
    EXPECT_EQ(event_view.entry(4).id(), 465);
    EXPECT_EQ(event_view.entry(4).idx(), 4);
    EXPECT_EQ(event_view.entry(4).cpu(), 4);
    EXPECT_EQ(event_view.entry(4).tid(), 1342);
    EXPECT_EQ(event_view.entry(5).id(), 466);
    EXPECT_EQ(event_view.entry(5).idx(), 5);
    EXPECT_EQ(event_view.entry(5).cpu(), 5);
    EXPECT_EQ(event_view.entry(5).tid(), 1342);
    EXPECT_EQ(event_view.entry(6).id(), 467);
    EXPECT_EQ(event_view.entry(6).idx(), 6);
    EXPECT_EQ(event_view.entry(6).cpu(), 6);
    EXPECT_EQ(event_view.entry(6).tid(), 1342);
    EXPECT_EQ(event_view.entry(7).id(), 468);
    EXPECT_EQ(event_view.entry(7).idx(), 7);
    EXPECT_EQ(event_view.entry(7).cpu(), 7);
    EXPECT_EQ(event_view.entry(7).tid(), 1342);
}

TEST(PerfDataParser, PerfThreadMapEvent)
{
    const std::array<std::uint8_t, 40> buffer = {
        0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto event_view = perf_data::parser::thread_map_event_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::thread_map_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 32);

    EXPECT_EQ(event_view.nr(), 1);
    EXPECT_EQ(event_view.entry(0).pid(), 1342);
}

TEST(PerfDataParser, PerfCpuMapEvent)
{
    const std::array<std::uint8_t, 40> buffer = {
        0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto event_view = perf_data::parser::cpu_map_event_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ(event_view.header().type(), perf_data::parser::cpu_map_event_view::event_type);
    EXPECT_EQ(event_view.header().misc(), 0);
    EXPECT_EQ(event_view.header().size(), 32);

    EXPECT_EQ(event_view.type(), perf_data::parser::cpu_map_type::mask);
    EXPECT_EQ(event_view.mask_data().nr(), 0);
    EXPECT_EQ(event_view.mask_data().long_size(), 0);
}

TEST(PerfDataParser, PerfHeaderBuildIdEvent)
{
    const std::array<std::uint8_t, 100> buffer = {
        0x00, 0x00, 0x00, 0x00, 0x02, 0x80, 0x64, 0x00, 0xff, 0xff, 0xff, 0xff, 0xeb, 0xc2, 0x58, 0x8d,
        0xf8, 0x7c, 0xb9, 0x5d, 0x0f, 0x91, 0x29, 0x20, 0x3b, 0xeb, 0xe6, 0x46, 0xed, 0xcd, 0x79, 0x30,
        0x14, 0x00, 0x00, 0x00, 0x2f, 0x74, 0x6d, 0x70, 0x2f, 0x62, 0x75, 0x69, 0x6c, 0x64, 0x2f, 0x69,
        0x6e, 0x6e, 0x65, 0x72, 0x2f, 0x44, 0x65, 0x62, 0x75, 0x67, 0x2f, 0x62, 0x75, 0x69, 0x6c, 0x64,
        0x2f, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};

    const auto event_view = perf_data::parser::header_build_id_event_view(std::as_bytes(std::span(buffer)), std::endian::little);

    EXPECT_EQ((int)event_view.header().type(), 0);
    EXPECT_EQ(event_view.header().misc(), 32770);
    EXPECT_EQ(event_view.header().size(), 100);

    EXPECT_EQ(event_view.pid(), 0xffffffff);
    EXPECT_EQ(event_view.build_id().size(), 20);
    EXPECT_EQ(event_view.filename(), "/tmp/build/inner/Debug/build/inner");
}
