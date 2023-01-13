
#pragma once

#include <cstdint>

#include <string>
#include <array>
#include <optional>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/utility.hpp>
#include <snail/etl/parser/records/identifier.hpp>

//
// Event records for event_trace_group::image
//

namespace snail::etl::parser {

// See https://learn.microsoft.com/en-us/windows/win32/etw/image-load
// or `Image_Load:Image` from wmicore.mof in WDK 10.0.22621.0
struct image_v2_load_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 3;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{ event_trace_group::process, 10, "load" }, // For an unknown reason, this is reported as in the process group
        event_identifier_group{ event_trace_group::image,    2, "unload" },
        event_identifier_group{ event_trace_group::image,    3, "dc_start" },
        event_identifier_group{ event_trace_group::image,    4, "dc_end" }
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;
    using extract_view_dynamic_base::buffer;

    inline auto image_base() const { return extract<std::uint64_t>(dynamic_offset(0, 0)); }
    inline auto image_size() const { return extract<std::uint64_t>(dynamic_offset(0, 1)); }

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 2)); }

    inline auto image_checksum() const { return extract<std::uint32_t>(dynamic_offset(4, 2)); }

    inline auto time_date_stamp() const { return extract<std::uint8_t>(dynamic_offset(8, 2)); }

    inline auto signature_level() const { return extract<std::uint8_t>(dynamic_offset(9, 2)); }
    inline auto signature_type() const { return extract<std::uint16_t>(dynamic_offset(10, 2)); }

    // inline auto reserved_0() const { return extract<std::uint32_t>(dynamic_offset(12, 2)); }

    inline auto default_base() const { return extract<std::uint64_t>(dynamic_offset(16, 2)); }

    // inline auto reserved_1() const { return extract<std::uint32_t>(dynamic_offset(16, 3)); }
    // inline auto reserved_2() const { return extract<std::uint32_t>(dynamic_offset(20, 3)); }
    // inline auto reserved_3() const { return extract<std::uint32_t>(dynamic_offset(24, 3)); }
    // inline auto reserved_4() const { return extract<std::uint32_t>(dynamic_offset(28, 3)); }
 
    inline auto file_name() const { return extract_u16string(dynamic_offset(32, 3), file_name_length); }

private:
    mutable std::optional<std::size_t> file_name_length;
};

} // namespace snail::etl::parser
