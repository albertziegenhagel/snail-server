
#include <array>

#include <gtest/gtest.h>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/etl/parser/records/kernel/perfinfo.hpp>
#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>

using namespace snail;

TEST(EtlDispatchEventObserver, Dispatch)
{
    etl::dispatching_event_observer observer;

    const auto file_header = etl::etl_file::header_data{
        .start_time           = {},
        .end_time             = {},
        .start_time_qpc_ticks = {},
        .qpc_frequency        = {},
        .pointer_size         = 8,
        .number_of_processors = {},
        .number_of_buffers    = {},
        .buffer_size          = {},
        .log_file_mode        = {},
        .compression_format   = common::ms_xca_compression_format::none};

    const std::array<std::uint8_t, 32> perfinfo_buffer_data = {
        0x02, 0x00, 0x11, 0xc0, 0x20, 0x00, 0x2e, 0x0f, 0x6d, 0x11, 0x06, 0x42, 0xcb, 0x02, 0x00, 0x00,
        0x8a, 0x35, 0x01, 0x2e, 0x03, 0xf8, 0xff, 0xff, 0x20, 0x67, 0x00, 0x00, 0x01, 0x00, 0x48, 0x00};

    const std::array<std::uint8_t, 98> image_buffer_data = {
        0x62, 0x00, 0x14, 0xc0, 0x00, 0x00, 0x02, 0x00, 0x20, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x1c, 0x18, 0x06, 0x42, 0xcb, 0x02, 0x00, 0x00, 0xd7, 0x75, 0xe6, 0xb3, 0x54, 0x25, 0x18, 0x4f,
        0x83, 0x0b, 0x27, 0x62, 0x73, 0x25, 0x60, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xe0, 0x2d, 0x03, 0xf8, 0xff, 0xff, 0x00, 0x70, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x6b, 0x00, 0x72, 0x00,
        0x6e, 0x00, 0x6c, 0x00, 0x6d, 0x00, 0x70, 0x00, 0x2e, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
        0x00, 0x00};

    const std::array<std::uint8_t, 144> log_disk_buffer_data = {
        0x02, 0x00, 0x02, 0xc0, 0x90, 0x00, 0x0c, 0x0b, 0x48, 0x04, 0x00, 0x00, 0xe0, 0x12, 0x00, 0x00,
        0x7f, 0x8c, 0xfe, 0xae, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x50, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xe0, 0x3f, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x3a, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd6, 0x54, 0x32, 0x01, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x0b, 0xfe, 0x03, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x54, 0x00, 0x46, 0x00, 0x53, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const std::array<std::uint8_t, 98> volume_mapping_data = {
        0x62, 0x00, 0x14, 0xc0, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x1e, 0x08, 0xef, 0xac, 0x00, 0x00, 0x00, 0x00, 0x91, 0xee, 0x79, 0x9b, 0xfd, 0xb5, 0xc0, 0x41,
        0xa2, 0x43, 0x42, 0x48, 0xe2, 0x66, 0xe9, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x5c, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x52, 0x00,
        0x6f, 0x00, 0x6f, 0x00, 0x74, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x43, 0x00, 0x3a, 0x00, 0x5c, 0x00,
        0x57, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x64, 0x00, 0x6f, 0x00, 0x77, 0x00, 0x73, 0x00, 0x5c, 0x00,
        0x00, 0x00};

    bool common_perfinfo_called = false;
    observer.register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>(
        [&common_perfinfo_called](const etl::etl_file::header_data& /*file_header*/,
                                  const etl::common_trace_header&                            header,
                                  const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
        {
            EXPECT_EQ(header.timestamp, 3072009310573);
            EXPECT_EQ(header.type, 46);
            EXPECT_EQ(header.buffer.size(), 16);

            EXPECT_EQ(event.instruction_pointer(), 18446735291273262474ULL);

            common_perfinfo_called = true;
        });
    bool variant_perfinfo_called = false;
    observer.register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>(
        [&variant_perfinfo_called](const etl::etl_file::header_data& /*file_header*/,
                                   const etl::any_group_trace_header&                         header,
                                   const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
        {
            const auto* full_header = std::get_if<etl::parser::perfinfo_trace_header_view>(&header);
            EXPECT_TRUE(full_header != nullptr);
            if(full_header != nullptr)
            {
                EXPECT_EQ(full_header->system_time(), 3072009310573);
                EXPECT_EQ(full_header->packet().type(), 46);
            }

            EXPECT_EQ(event.instruction_pointer(), 18446735291273262474ULL);

            variant_perfinfo_called = true;
        });

    bool common_image_called = false;
    observer.register_event<etl::parser::image_id_v2_info_event_view>(
        [&common_image_called](const etl::etl_file::header_data& /*file_header*/,
                               const etl::common_trace_header&                 header,
                               const etl::parser::image_id_v2_info_event_view& event)
        {
            EXPECT_EQ(header.timestamp, 3072009312284);
            EXPECT_EQ(header.type, 0);
            EXPECT_EQ(header.buffer.size(), 48);

            EXPECT_EQ(event.image_size(), 17068032);

            common_image_called = true;
        });
    bool variant_image_called = false;
    observer.register_event<etl::parser::image_id_v2_info_event_view>(
        [&variant_image_called](const etl::etl_file::header_data& /*file_header*/,
                                const etl::any_guid_trace_header&               header,
                                const etl::parser::image_id_v2_info_event_view& event)
        {
            const auto* full_header = std::get_if<etl::parser::full_header_trace_header_view>(&header);
            EXPECT_TRUE(full_header != nullptr);
            if(full_header != nullptr)
            {
                EXPECT_EQ(full_header->timestamp(), 3072009312284);
                EXPECT_EQ(full_header->trace_class().type(), 0);
            }

            EXPECT_EQ(event.image_size(), 17068032);

            variant_image_called = true;
        });

    bool variant_unknown_guid_called = false;
    observer.register_unknown_event(
        [&variant_unknown_guid_called](const etl::etl_file::header_data& /*file_header*/,
                                       const etl::any_guid_trace_header& header,
                                       const std::span<const std::byte>& event_data)
        {
            const auto* full_header = std::get_if<etl::parser::full_header_trace_header_view>(&header);
            EXPECT_TRUE(full_header != nullptr);
            if(full_header != nullptr)
            {
                EXPECT_EQ(full_header->timestamp(), 2901346334);
                EXPECT_EQ(full_header->trace_class().type(), 35);
            }

            EXPECT_EQ(event_data.size(), 50);

            variant_unknown_guid_called = true;
        });

    bool variant_unknown_group_called = false;
    observer.register_unknown_event(
        [&variant_unknown_group_called](const etl::etl_file::header_data& /*file_header*/,
                                        const etl::any_group_trace_header& header,
                                        const std::span<const std::byte>&  event_data)
        {
            const auto* system_header = std::get_if<etl::parser::system_trace_header_view>(&header);
            EXPECT_TRUE(system_header != nullptr);
            if(system_header != nullptr)
            {
                EXPECT_EQ(system_header->system_time(), 2935917695);
                EXPECT_EQ(system_header->packet().type(), 12);
            }

            EXPECT_EQ(event_data.size(), 112);

            variant_unknown_group_called = true;
        });

    {
        common_perfinfo_called = variant_perfinfo_called = false;
        common_image_called = variant_image_called = false;
        variant_unknown_guid_called                = false;
        variant_unknown_group_called               = false;

        const auto buffer       = std::as_bytes(std::span(perfinfo_buffer_data));
        const auto trace_header = etl::parser::perfinfo_trace_header_view(buffer.subspan(0, etl::parser::perfinfo_trace_header_view::static_size));
        const auto user_buffer  = buffer.subspan(etl::parser::perfinfo_trace_header_view::static_size);

        observer.handle(file_header, trace_header, user_buffer);

        EXPECT_TRUE(common_perfinfo_called);
        EXPECT_TRUE(variant_perfinfo_called);
        EXPECT_FALSE(common_image_called);
        EXPECT_FALSE(variant_image_called);
        EXPECT_FALSE(variant_unknown_guid_called);
        EXPECT_FALSE(variant_unknown_group_called);
    }

    {
        common_perfinfo_called = variant_perfinfo_called = false;
        common_image_called = variant_image_called = false;
        variant_unknown_guid_called                = false;
        variant_unknown_group_called               = false;

        const auto buffer       = std::as_bytes(std::span(image_buffer_data));
        const auto trace_header = etl::parser::full_header_trace_header_view(buffer.subspan(0, etl::parser::full_header_trace_header_view::static_size));
        const auto user_buffer  = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

        observer.handle(file_header, trace_header, user_buffer);

        EXPECT_FALSE(common_perfinfo_called);
        EXPECT_FALSE(variant_perfinfo_called);
        EXPECT_TRUE(common_image_called);
        EXPECT_TRUE(variant_image_called);
        EXPECT_FALSE(variant_unknown_guid_called);
        EXPECT_FALSE(variant_unknown_group_called);
    }

    {
        common_perfinfo_called = variant_perfinfo_called = false;
        common_image_called = variant_image_called = false;
        variant_unknown_guid_called                = false;
        variant_unknown_group_called               = false;

        const auto buffer       = std::as_bytes(std::span(volume_mapping_data));
        const auto trace_header = etl::parser::full_header_trace_header_view(buffer.subspan(0, etl::parser::full_header_trace_header_view::static_size));
        const auto user_buffer  = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

        observer.handle(file_header, trace_header, user_buffer);

        EXPECT_FALSE(common_perfinfo_called);
        EXPECT_FALSE(variant_perfinfo_called);
        EXPECT_FALSE(common_image_called);
        EXPECT_FALSE(variant_image_called);
        EXPECT_TRUE(variant_unknown_guid_called);
        EXPECT_FALSE(variant_unknown_group_called);
    }

    {
        common_perfinfo_called = variant_perfinfo_called = false;
        common_image_called = variant_image_called = false;
        variant_unknown_guid_called                = false;
        variant_unknown_group_called               = false;

        const auto buffer       = std::as_bytes(std::span(log_disk_buffer_data));
        const auto trace_header = etl::parser::system_trace_header_view(buffer.subspan(0, etl::parser::system_trace_header_view::static_size));
        const auto user_buffer  = buffer.subspan(etl::parser::system_trace_header_view::static_size);

        observer.handle(file_header, trace_header, user_buffer);

        EXPECT_FALSE(common_perfinfo_called);
        EXPECT_FALSE(variant_perfinfo_called);
        EXPECT_FALSE(common_image_called);
        EXPECT_FALSE(variant_image_called);
        EXPECT_FALSE(variant_unknown_guid_called);
        EXPECT_TRUE(variant_unknown_group_called);
    }
}
