
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// Event records for event_trace_group::config
//

namespace snail::etl::parser {

enum class drive_type : std::uint32_t
{
    partition          = 1,
    volume             = 2,
    extended_partition = 3
};

// See https://learn.microsoft.com/en-us/windows/win32/etw/systemconfig-phydisk
// or `SystemConfig_V2_PhyDisk:SystemConfig_V2` from wmicore.mof in WDK 10.0.22621.0
struct system_config_v2_physical_disk_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::config, 11, "physical disk"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto disk_number() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto bytes_per_sector() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }
    inline auto sectors_per_track() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); }
    inline auto tracks_per_cylinder() const { return extract<std::uint32_t>(dynamic_offset(12, 0)); }
    inline auto cylinders() const { return extract<std::uint64_t>(dynamic_offset(16, 0)); }
    inline auto scsi_port() const { return extract<std::uint32_t>(dynamic_offset(24, 0)); }
    inline auto scsi_path() const { return extract<std::uint32_t>(dynamic_offset(28, 0)); }
    inline auto scsi_target() const { return extract<std::uint32_t>(dynamic_offset(32, 0)); }
    inline auto scsi_lun() const { return extract<std::uint32_t>(dynamic_offset(36, 0)); }
    inline auto manufacturer() const { return extract_u16string(dynamic_offset(40, 0), manufacturer_length); } // length = 256
    inline auto partition_count() const { return extract<std::uint32_t>(dynamic_offset(552, 0)); }
    inline auto write_cache_enabled() const { return extract<std::uint8_t>(dynamic_offset(556, 0)); }
    // inline auto pad() const { return extract<std::uint8_t>(dynamic_offset(557, 0)); }
    inline auto boot_drive_letter() const { return extract_u16string(dynamic_offset(558, 0), boot_drive_letter_length); } // length = 3
    inline auto spare() const { return extract_u16string(dynamic_offset(564, 0), spare_length); }                         // length = 2

    static inline constexpr std::size_t static_size = 568;

private:
    mutable std::optional<std::size_t> manufacturer_length;
    mutable std::optional<std::size_t> boot_drive_letter_length;
    mutable std::optional<std::size_t> spare_length;
};

// See https://learn.microsoft.com/en-us/windows/win32/etw/systemconfig-logdisk
// or `SystemConfig_V2_LogDisk:SystemConfig_V2` from wmicore.mof in WDK 10.0.22621.0
struct system_config_v2_logical_disk_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::config, 12, "logical disk"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto start_offset() const { return extract<std::uint64_t>(dynamic_offset(0, 0)); }
    inline auto partition_size() const { return extract<std::uint64_t>(dynamic_offset(8, 0)); }
    inline auto disk_number() const { return extract<std::uint32_t>(dynamic_offset(16, 0)); }
    inline auto size() const { return extract<std::uint32_t>(dynamic_offset(20, 0)); }
    inline auto drive_type() const { return extract<parser::drive_type>(dynamic_offset(24, 0)); }
    inline auto drive_letter() const { return extract_u16string(dynamic_offset(28, 0), drive_letter_length); } // length = 4
    // inline auto pad1() const { return extract<std::uint32_t>(dynamic_offset(36, 0)); }
    inline auto partition_number() const { return extract<std::uint32_t>(dynamic_offset(40, 0)); }
    inline auto sectors_per_cluster() const { return extract<std::uint32_t>(dynamic_offset(44, 0)); }
    inline auto bytes_per_sector() const { return extract<std::uint32_t>(dynamic_offset(48, 0)); }
    // inline auto pad2() const { return extract<std::uint32_t>(dynamic_offset(52, 0)); }
    inline auto number_of_free_clusters() const { return extract<std::int64_t>(dynamic_offset(56, 0)); }
    inline auto total_number_of_clusters() const { return extract<std::int64_t>(dynamic_offset(64, 0)); }
    inline auto file_system() const { return extract_u16string(dynamic_offset(72, 0), file_system_length); } // length = 16
    inline auto volume_extent() const { return extract<std::int32_t>(dynamic_offset(104, 0)); }
    // inline auto pad3() const { return extract<std::uint32_t>(dynamic_offset(108, 0)); }

    static inline constexpr std::size_t static_size = 112;

private:
    mutable std::optional<std::size_t> drive_letter_length;
    mutable std::optional<std::size_t> file_system_length;
};

} // namespace snail::etl::parser
