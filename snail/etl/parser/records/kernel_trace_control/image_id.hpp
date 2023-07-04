
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/common/guid.hpp>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

namespace snail::etl::parser {

constexpr inline auto image_id_task_guid = common::guid{
    0xb3e675d7, 0x2554, 0x4f18, {0x83, 0x0b, 0x27, 0x62, 0x73, 0x25, 0x60, 0xde}
};

struct image_id_v2_info_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{image_id_task_guid, 0, "info"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto image_base() const { return extract_pointer(dynamic_offset(0, 0)); }
    inline auto image_size() const { return extract_pointer(dynamic_offset(0, 1)); }

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 2)); }

    inline auto time_date_stamp() const { return extract<std::uint32_t>(dynamic_offset(4, 2)); }

    inline auto original_file_name() const { return extract_u16string(dynamic_offset(8, 2), original_file_name_length); }

private:
    mutable std::optional<std::size_t> original_file_name_length;
};

} // namespace snail::etl::parser
