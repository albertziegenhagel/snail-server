
#include <cstddef>

#include <gtest/gtest.h>

#include <binarystream/binarystream.hpp>

namespace pru = perfreader::utility;



TEST(BinaryStream, Init)
{
    pru::binary_stream_parser stream(R"(C:\Users\aziegenhagel\source\perfreader\tests\data\sc.user_aux.etl)", std::endian::little);

    wmi_buffer_header header;

    std::set<std::uint8_t> header_types;
    std::size_t headers = 0;
    while(stream.good())
    {
        parse(stream, header);
        ++headers;
        const auto payload_size = header.wnode.saved_offset - sizeof(wmi_buffer_header);
        const auto padding_size = header.wnode.buffer_size - header.wnode.saved_offset;

        assert(header.buffer_type == etw_buffer_type::header ||
               header.buffer_type == etw_buffer_type::generic);

        const auto payload_start_pos = stream.tell_position();

        const auto descriptor = stream.read<std::uint32_t>();

        stream.set_position(payload_start_pos);

        constexpr std::uint32_t trace_header_flag             = 0x80000000;
        constexpr std::uint32_t trace_header_event_trace_flag = 0x40000000;

        constexpr std::uint32_t trace_header_enum_mask        = 0x00FF0000;

        const auto is_trace_header = (descriptor & trace_header_flag) != 0;

        if(!is_trace_header)
        {
            std::cout << "(WARNING) skip unknown buffer" << "\n";
            stream.set_position(payload_start_pos);
            stream.skip(payload_size);
            stream.skip(padding_size);
            continue;
        }

        [[maybe_unused]] const auto is_event_trace  = (descriptor & trace_header_event_trace_flag) != 0;

        const auto header_type = static_cast<trace_header_type>((descriptor & trace_header_enum_mask) >> 16);

        switch(header_type)
        {
        case trace_header_type::system32:
        case trace_header_type::system64:
            {
                system_trace_header trace_header;
                parse(stream, trace_header);

                trace_header.version;
                trace_header.packet.group;
                trace_header.packet.type;

                int stop = 0;
            }
            break;
        case trace_header_type::perfinfo64:
            {
                perfinfo_trace_header trace_header;
                parse(stream, trace_header);
            }
            break;
        default:
            std::cout << "(WARNING) skip unknown header: " << (int) header_type << std::endl;
            break;
        }

        stream.set_position(payload_start_pos);
        stream.skip(payload_size);
        stream.skip(padding_size);
    }

    EXPECT_EQ(header.wnode.buffer_size, 1);
}