
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

//
// Event records for the visual studio diagnostics hub. See
//  <VSROOT>\Common7\IDE\CommonExtensions\Platform\DiagnosticsHub\Manifests\Microsoft-VisualStudio-Performance-DiagnosticsHub.manifest.xml
//

namespace snail::etl::parser {

constexpr inline auto vs_diagnostics_hub_guid = guid{
    0x9e5f9046, 0x43c6, 0x4f62, {0xba, 0x13, 0x7b, 0x19, 0x89, 0x62, 0x53, 0xff}
};

enum class counter_type : std::uint32_t
{
    unknown      = 0,
    PrivateBytes = 1,
    CPU          = 2
};

enum class target_start_reason : std::uint32_t
{
    app_package_launch = 0,
    attach_to_process  = 1
};

enum class machine_architecture : std::uint32_t
{
    x86   = 0,
    amd64 = 1,
    arm   = 2,
    arm64 = 4,
    phone = 256
};

struct vs_diagnostics_hub_target_profiling_started_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 2;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{vs_diagnostics_hub_guid, 1, "target profiling started"}
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto start_reason() const { return extract<target_start_reason>(dynamic_offset(4, 0)); }
    inline auto timestamp() const { return extract<std::uint64_t>(dynamic_offset(8, 0)); }
};

struct vs_diagnostics_hub_target_profiling_stopped_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 1;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{vs_diagnostics_hub_guid, 2, "target profiling stopped"}
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 0)); }
    inline auto timestamp() const { return extract<std::uint64_t>(dynamic_offset(4, 0)); }
};

struct vs_diagnostics_hub_machine_info_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{vs_diagnostics_hub_guid, 5, "machine info"}
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto name() const { return extract_u16string(dynamic_offset(0, 0), name_length); }
    inline auto os_description() const { return extract_u16string(dynamic_offset(name().size() * 2 + 2, 0), os_description_length); }
    inline auto architecture() const { return extract<machine_architecture>(dynamic_offset(name().size() * 2 + 2 + os_description().size() * 2 + 2, 0)); }

private:
    mutable std::optional<std::size_t> name_length;
    mutable std::optional<std::size_t> os_description_length;
};

struct vs_diagnostics_hub_counter_info_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{vs_diagnostics_hub_guid, 6, "counter info"}
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto counter() const { return extract<counter_type>(dynamic_offset(0, 0)); }
    inline auto timestamp() const { return extract<std::uint64_t>(dynamic_offset(4, 0)); }
    inline auto value() const { return extract<double>(dynamic_offset(12, 0)); }
};

struct vs_diagnostics_hub_mark_info_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{vs_diagnostics_hub_guid, 7, "mark info"}
    };

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto message() const { return extract_u16string(dynamic_offset(0, 0), message_length); }

private:
    mutable std::optional<std::size_t> message_length;
};

} // namespace snail::etl::parser
