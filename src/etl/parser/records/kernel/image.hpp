
#pragma once

#include <cstdint>

#include <string>
#include <array>
#include <optional>

#include "etl/parser/extract.hpp"
#include "etl/parser/utility.hpp"

//
// Event records for event_trace_group::image
//

namespace perfreader::etl::parser {

// See https://learn.microsoft.com/en-us/windows/win32/etw/image-load
// or `Image_Load:Image` from wmicore.mof in WDK 10.0.22621.0
struct image_v2_load_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        load     = 10,
        unload   = 2,
        dc_start = 3,
        dc_end   = 4,
    };
    // `load` is in event_trace_group::process?? WHY?
    static inline constexpr std::array<std::uint8_t, 4>  event_types    = {2, 3, 4, 10};
    static inline constexpr std::array<std::uint16_t, 2> event_versions = {3};

    using extract_view_dynamic_base::extract_view_dynamic_base;

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

} // namespace perfreader::etl::parser
