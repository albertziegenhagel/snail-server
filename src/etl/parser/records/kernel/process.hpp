
#pragma once

#include <cstdint>

#include <string>
#include <array>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// event records for event_trace_group::process

// See https://learn.microsoft.com/en-us/windows/win32/etw/process-typegroup1
struct event_process_v3_type_group1
{
    // 1=Load, 2=Unload, 3=DCStart, 4=DCEnd, 39=Defunct
    static inline constexpr std::array<std::uint8_t, 5> event_types = {1, 2, 3, 4, 39};
    static inline constexpr std::uint8_t event_version              = 3;

    pointer unique_process_key;
    std::uint32_t process_id;
    std::uint32_t parent_id;
    std::uint32_t session_id;
    std::int32_t exit_status;
    pointer directory_table_base;
    sid user_sid;
    std::string image_filename;
    std::u16string command_line;
};

std::size_t parse(utility::binary_stream_parser& stream, event_process_v3_type_group1& data);

// `Process_V4_TypeGroup1:Process_V4` from wmicore.mof in WDK 10.0.22621.0
struct process_v4_type_group1_event_view : private extract_view_dynamic_base
{
    enum class event_type : std::uint8_t
    {
        load     = 1,
        unload   = 2,
        dc_start = 3,
        dc_end   = 4,
        defunct  = 39,
    };
    static inline constexpr std::array<std::uint8_t, 5> event_types     = {1, 2, 3, 4, 39};
    static inline constexpr std::array<std::uint16_t, 2> event_versions = {4};

    using extract_view_dynamic_base::extract_view_dynamic_base;

    inline auto unique_process_key() const { return extract_pointer(dynamic_offset(0, 0)); }

    inline auto process_id() const { return extract<std::uint32_t>(dynamic_offset(0, 1)); }
    inline auto parent_id() const { return extract<std::uint32_t>(dynamic_offset(4, 1)); }
    inline auto session_id() const { return extract<std::uint32_t>(dynamic_offset(8, 1)); }

    inline auto exit_status() const { return extract<std::int32_t>(dynamic_offset(12, 1)); }

    inline auto directory_table_base() const { return extract_pointer(dynamic_offset(16, 1)); }

    inline auto flags() const { return extract<std::uint32_t>(dynamic_offset(16, 2)); }

    // NOTE: No clue why the following 16 bits exist. They are not given in the wmicore.mof
    //       description. Maybe they belong to the following SID type??
    // inline auto unknown_0() const { return extract<std::uint64_t>(dynamic_offset(20, 2)); }
    // inline auto unknown_1() const { return extract<std::uint64_t>(dynamic_offset(28, 2)); }

    inline auto user_sid() const { return sid_view(buffer_.subspan(dynamic_offset(36, 2))); }

    inline auto image_filename() const { return extract_string(dynamic_offset(36 + user_sid().dynamic_size(), 2), image_filename_length); }
    inline auto command_line() const { return extract_u16string(dynamic_offset(36 + user_sid().dynamic_size(), 2) + image_filename().size() + 1, command_line_length); }
    inline auto package_full_name() const { return extract_u16string(dynamic_offset(36 + user_sid().dynamic_size(), 2) + image_filename().size() + 1 + command_line().size() + 2, package_full_name_length); }
    inline auto application_id() const { return extract_u16string(dynamic_offset(36 + user_sid().dynamic_size(), 2) + image_filename().size() + 1 + command_line().size() + 2 + package_full_name().size() + 2, application_id_length); }

private:
    mutable std::optional<std::size_t> image_filename_length;
    mutable std::optional<std::size_t> command_line_length;
    mutable std::optional<std::size_t> package_full_name_length;
    mutable std::optional<std::size_t> application_id_length;
};

} // namespace perfreader::etl::parser
