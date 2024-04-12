
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

// or `SystemConfig_V3_CPU:SystemConfig_V3` from wmicore.mof in WDK 10.0.22621.0
struct system_config_v3_cpu_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 3;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::config, 10, "cpu"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto mhz() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto number_of_processors() const { return extract<std::uint32_t>(dynamic_offset(4, 0)); }
    inline auto mem_size() const { return extract<std::uint32_t>(dynamic_offset(8, 0)); }
    inline auto page_size() const { return extract<std::uint32_t>(dynamic_offset(12, 0)); }
    inline auto allocation_granularity() const { return extract<std::uint32_t>(dynamic_offset(16, 0)); }
    inline auto computer_name() const { return extract_u16string(dynamic_offset(20, 0), computer_name_length); } // length = 256
    inline auto domain_name() const { return extract_u16string(dynamic_offset(532, 0), domain_name_length); }    // length = 134
    inline auto hyper_threading_flag() const { return extract_pointer(dynamic_offset(800, 0)); }
    inline auto highest_user_address() const { return extract_pointer(dynamic_offset(800, 1)); }
    inline auto processor_architecture() const { return extract<std::uint16_t>(dynamic_offset(800, 2)); }
    inline auto processor_level() const { return extract<std::uint16_t>(dynamic_offset(802, 2)); }
    inline auto processor_revision() const { return extract<std::uint16_t>(dynamic_offset(804, 2)); }
    inline auto pae_enabled() const { return extract<std::uint8_t>(dynamic_offset(806, 2)); }
    inline auto nx_enabled() const { return extract<std::uint8_t>(dynamic_offset(807, 2)); }
    inline auto memory_speed() const { return extract<std::uint32_t>(dynamic_offset(808, 2)); }

    inline std::size_t dynamic_size() const { return dynamic_offset(812, 2); }

private:
    mutable std::optional<std::size_t> computer_name_length;
    mutable std::optional<std::size_t> domain_name_length;
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

    inline std::size_t dynamic_size() const { return static_size; }

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

    inline std::size_t dynamic_size() const { return static_size; }

private:
    mutable std::optional<std::size_t> drive_letter_length;
    mutable std::optional<std::size_t> file_system_length;
};

// or `SystemConfig_PnP:SystemConfig` from wmicore.mof in WDK 10.0.22621.0
struct system_config_v5_pnp_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 5;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_group{event_trace_group::config, 22, "pnp"},
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto class_guid() const { return guid_view(buffer().subspan(0, guid_view::static_size)); }
    inline auto upper_filters_count() const { return extract<std::uint32_t>(dynamic_offset(16, 0)); }
    inline auto lower_filters_count() const { return extract<std::uint32_t>(dynamic_offset(20, 0)); }
    inline auto dev_status() const { return extract<std::uint32_t>(dynamic_offset(24, 0)); }
    inline auto dev_problem() const { return extract<std::uint32_t>(dynamic_offset(28, 0)); }
    inline auto device_id() const { return extract_u16string(dynamic_offset(32, 0), device_id_length); }
    inline auto device_description() const { return extract_u16string(dynamic_offset(32 + device_id().size() * 2 + 2, 0), device_description_length); }
    inline auto friendly_name() const { return extract_u16string(dynamic_offset(32 + device_id().size() * 2 + device_description().size() * 2 + 4, 0), friendly_name_length); }
    inline auto pdo_name() const { return extract_u16string(dynamic_offset(32 + device_id().size() * 2 + device_description().size() * 2 + friendly_name().size() * 2 + 6, 0), pdo_name_length); }
    inline auto service_name() const { return extract_u16string(dynamic_offset(32 + device_id().size() * 2 + device_description().size() * 2 + friendly_name().size() * 2 + pdo_name().size() * 2 + 8, 0), service_name_length); }
    // inline auto upper_filters() const { return extract_u16string(dynamic_offset(32, 0)); }
    // inline auto lower_filters() const { return extract_u16string(dynamic_offset(32, 0)); }

    inline std::size_t dynamic_size() const
    {
        auto offset = dynamic_offset(32 + device_id().size() * 2 + device_description().size() * 2 + friendly_name().size() * 2 + pdo_name().size() * 2 + service_name().size() * 2 + 10, 0);
        for(std::uint32_t i = 0; i < upper_filters_count(); ++i)
        {
            offset += (common::parser::detect_string_size<char16_t>(buffer(), offset) * 2 + 2);
        }
        for(std::uint32_t i = 0; i < lower_filters_count(); ++i)
        {
            offset += (common::parser::detect_string_size<char16_t>(buffer(), offset) * 2 + 2);
        }
        return offset;
    }

private:
    mutable std::optional<std::size_t> device_id_length;
    mutable std::optional<std::size_t> device_description_length;
    mutable std::optional<std::size_t> friendly_name_length;
    mutable std::optional<std::size_t> pdo_name_length;
    mutable std::optional<std::size_t> service_name_length;
};

} // namespace snail::etl::parser
