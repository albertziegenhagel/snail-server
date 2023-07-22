
#include <array>

#include <gtest/gtest.h>

#include <snail/perf_data/detail/attributes_database.hpp>

#include <snail/perf_data/parser/event.hpp>

using namespace snail::perf_data;
using namespace snail::perf_data::detail;

TEST(EventAttributesId, Offset)
{
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000101110111"))),
              0);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000111"))),
              std::nullopt);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000000"))),
              0);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000001"))),
              8);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000010"))),
              8);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000100"))),
              8);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001000"))),
              8);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000011"))),
              16);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000101"))),
              16);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001001"))),
              16);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000110"))),
              16);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001010"))),
              16);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000111"))),
              24);
    EXPECT_EQ(calculate_id_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001111"))),
              32);
}

TEST(EventAttributesId, BackOffset)
{
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000100100111"))),
              8);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000111"))),
              std::nullopt);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000000"))),
              8);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000000"))),
              8);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000011000000"))),
              16);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000001001000000"))),
              16);
    EXPECT_EQ(calculate_id_back_offset(parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000001011000000"))),
              24);
}

TEST(EventAttributesDatabase, InvalidEmpty)
{
    event_attributes_database database;

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, InvalidReadFormat)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100110111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000000")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, ValidSingle)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_NO_THROW(database.validate());
}

TEST(EventAttributesDatabase, NonMatchingOffset)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000010")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, NonMatchingBackOffset)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000000")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000001011000000")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, NonMatchingSampleAll)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100100000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, NonMatchingReadFormat)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000101")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_THROW(database.validate(), std::runtime_error);
}

TEST(EventAttributesDatabase, ValidMultiple)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000100")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001000")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100000000")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = {}}
    };

    EXPECT_NO_THROW(database.validate());
}

TEST(EventAttributesDatabase, GetMainOnly)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-1"}
    };

    EXPECT_NO_THROW(database.validate());

    const std::array<std::uint8_t, 48> buffer = {
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x70, 0x65, 0x72, 0x66, 0x2d, 0x65, 0x78, 0x65,
        0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer)));
    EXPECT_EQ(attr.name, "attr-1");
}

TEST(EventAttributesDatabase, GetNonSampleNoId)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001000100")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100100000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-1"},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000001001000")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100100000011011100000000")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-2"}
    };

    EXPECT_NO_THROW(database.validate());

    const std::array<std::uint8_t, 48> buffer = {
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x3e, 0x05, 0x00, 0x00, 0x3e, 0x05, 0x00, 0x00, 0x70, 0x65, 0x72, 0x66, 0x2d, 0x65, 0x78, 0x65,
        0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer)));
    EXPECT_EQ(attr.name, "attr-1");
}

TEST(EventAttributesDatabase, GetNonSampleAllId)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-1"},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-2"}
    };

    database.id_to_attributes[1] = &database.all_attributes[0];
    database.id_to_attributes[4] = &database.all_attributes[1];

    EXPECT_NO_THROW(database.validate());

    {
        // id == 0 is always the "main" attribute
        const std::array<std::uint8_t, 16> buffer = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-1");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-1");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-2");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        EXPECT_THROW(database.get_event_attributes(std::endian::little, parser::event_type::comm, std::as_bytes(std::span(buffer))),
                     std::runtime_error);
    }
}

TEST(EventAttributesDatabase, GetSampleAllId)
{
    event_attributes_database database;

    database.all_attributes = {
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-1"},
        parser::event_attributes{
                                 .type          = parser::attribute_type::hardware,
                                 .sample_format = parser::sample_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000010000000100100111")),
                                 .read_format   = parser::read_format_flags(std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100")),
                                 .flags         = parser::attribute_flags(std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011")),
                                 .precise_ip    = parser::skid_constraint_type::can_have_arbitrary_skid,
                                 .name          = "attr-2"}
    };

    database.id_to_attributes[1] = &database.all_attributes[0];
    database.id_to_attributes[4] = &database.all_attributes[1];

    EXPECT_NO_THROW(database.validate());

    {
        // id == 0 is always the "main" attribute
        const std::array<std::uint8_t, 16> buffer = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::sample, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-1");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::sample, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-1");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        const auto& attr = database.get_event_attributes(std::endian::little, parser::event_type::sample, std::as_bytes(std::span(buffer)));
        EXPECT_EQ(attr.name, "attr-2");
    }
    {
        const std::array<std::uint8_t, 16> buffer = {
            0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        EXPECT_THROW(database.get_event_attributes(std::endian::little, parser::event_type::sample, std::as_bytes(std::span(buffer))),
                     std::runtime_error);
    }
}
