
#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/records/identifier.hpp>
#include <snail/etl/parser/utility.hpp>

namespace snail::etl::parser {

constexpr inline auto system_config_ex_guid = common::guid{
    0x9b79ee91, 0xb5fd, 0x41c0, {0xa2, 0x43, 0x42, 0x48, 0xe2, 0x66, 0xe9, 0xd0}
};

struct system_config_ex_v0_build_info_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{system_config_ex_guid, 32, "build_info"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto install_date() const { return extract<std::uint64_t>(dynamic_offset(0, 0)); }

    inline auto build_lab() const { return extract_u16string(dynamic_offset(8, 0), build_lab_name_length); }

    inline auto product_name() const { return extract_u16string(dynamic_offset(8, 0) + build_lab().size() * 2 + 2, product_name_name_length); }

private:
    mutable std::optional<std::size_t> build_lab_name_length;
    mutable std::optional<std::size_t> product_name_name_length;
};

struct system_config_ex_v0_system_paths_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{system_config_ex_guid, 33, "system_paths"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto system_directory() const { return extract_u16string(dynamic_offset(0, 0), system_directory_name_length); }

    inline auto system_windows_directory() const { return extract_u16string(dynamic_offset(0, 0) + system_directory().size() * 2 + 2, system_windows_directory_name_length); }

private:
    mutable std::optional<std::size_t> system_directory_name_length;
    mutable std::optional<std::size_t> system_windows_directory_name_length;
};

struct system_config_ex_v0_volume_mapping_event_view : private extract_view_dynamic_base
{
    static inline constexpr std::uint16_t event_version = 0;
    static inline constexpr auto          event_types   = std::array{
        event_identifier_guid{system_config_ex_guid, 35, "volume_mapping"}
    };

    using extract_view_dynamic_base::buffer;
    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto nt_path() const { return extract_u16string(dynamic_offset(0, 0), nt_path_name_length); }

    inline auto dos_path() const { return extract_u16string(dynamic_offset(0, 0) + nt_path().size() * 2 + 2, dos_path_name_length); }

private:
    mutable std::optional<std::size_t> nt_path_name_length;
    mutable std::optional<std::size_t> dos_path_name_length;
};

} // namespace snail::etl::parser
