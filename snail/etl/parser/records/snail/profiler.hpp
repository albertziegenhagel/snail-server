
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

constexpr inline auto snail_profiler_guid = common::guid{
    0x460b'83b6, 0xfc11, 0x481b, {0xb7, 0xaa, 0x40, 0x38, 0xca, 0x4c, 0x4c, 0x48}
};

struct snail_profiler_profile_target_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::string_view event_name    = "SnailProf-ProfTarget";
    static inline constexpr std::uint16_t    event_version = 0;
    static inline constexpr auto             event_types   = std::array{
        event_identifier_guid{snail_profiler_guid, 1, "ProfTarget"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }

    static inline constexpr std::size_t static_size = 4;

    inline std::size_t dynamic_size() const { return static_size; }
};

} // namespace snail::etl::parser
