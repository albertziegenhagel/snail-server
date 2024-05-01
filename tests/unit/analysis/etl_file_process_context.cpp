
#include <gtest/gtest.h>

#include <snail/analysis/detail/etl_file_process_context.hpp>

#include <snail/etl/parser/records/kernel/config.hpp>
#include <snail/etl/parser/records/kernel/image.hpp>
#include <snail/etl/parser/records/kernel/perfinfo.hpp>
#include <snail/etl/parser/records/kernel/process.hpp>
#include <snail/etl/parser/records/kernel/stackwalk.hpp>
#include <snail/etl/parser/records/kernel/thread.hpp>

#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>
#include <snail/etl/parser/records/kernel_trace_control/system_config_ex.hpp>

#include <snail/etl/parser/records/snail/profiler.hpp>
#include <snail/etl/parser/records/visual_studio/diagnostics_hub.hpp>

#include <snail/common/cast.hpp>

#include "helpers.hpp"

using namespace snail;
using namespace snail::analysis::detail;

using namespace snail::detail::tests;

namespace {

void set_trace_header_event_info(std::span<std::byte>           buffer,
                                 etl::parser::trace_header_type header_type,
                                 std::size_t                    event_size,
                                 std::uint16_t                  event_version,
                                 etl::parser::event_trace_group event_group,
                                 std::uint8_t                   event_type)
{
    set_at(buffer, 0, event_version);                                                                                                           // version
    set_at(buffer, 2, std::to_underlying(header_type));                                                                                         // header_type
    set_at(buffer, 3, etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag); // header_flags

    set_at(buffer, 4, common::narrow_cast<std::uint16_t>(etl::parser::system_trace_header_view::static_size + event_size)); // packet.size (Not required)
    set_at(buffer, 6, event_type);                                                                                          // packet.type
    set_at(buffer, 7, std::to_underlying(event_group));                                                                     // packet.group
}

auto make_system_trace_header(const etl::etl_file::header_data& file_header,
                              std::span<std::byte>              buffer,
                              std::uint64_t                     timestamp,
                              std::size_t                       event_size,
                              std::uint16_t                     event_version,
                              etl::parser::event_trace_group    event_group,
                              std::uint8_t                      event_type)
{
    set_trace_header_event_info(buffer, file_header.pointer_size == 4 ? etl::parser::trace_header_type::system32 : etl::parser::trace_header_type::system64, event_size, event_version, event_group, event_type);

    set_at(buffer, 16, timestamp); // system_time

    return etl::parser::system_trace_header_view(buffer);
}

auto make_perfinfo_trace_header(const etl::etl_file::header_data& file_header,
                                std::span<std::byte>              buffer,
                                std::uint64_t                     timestamp,
                                std::size_t                       event_size,
                                std::uint16_t                     event_version,
                                etl::parser::event_trace_group    event_group,
                                std::uint8_t                      event_type)
{
    set_trace_header_event_info(buffer, file_header.pointer_size == 4 ? etl::parser::trace_header_type::perfinfo32 : etl::parser::trace_header_type::perfinfo64, event_size, event_version, event_group, event_type);

    set_at(buffer, 8, timestamp); // system_time

    return etl::parser::perfinfo_trace_header_view(buffer);
}

auto make_full_trace_header(const etl::etl_file::header_data& file_header,
                            std::span<std::byte>              buffer,
                            std::uint64_t                     timestamp,
                            std::size_t                       event_size,
                            std::uint16_t                     event_version,
                            const common::guid&               provider_guid,
                            std::uint8_t                      event_type)
{
    set_at(buffer, 0, common::narrow_cast<std::uint16_t>(event_size));                                                                                                     // size
    set_at(buffer, 2, std::to_underlying(file_header.pointer_size == 4 ? etl::parser::trace_header_type::full_header32 : etl::parser::trace_header_type::full_header64));  // header_type
    set_at(buffer, 3, static_cast<std::uint8_t>(etl::parser::generic_trace_marker::trace_header_flag | etl::parser::generic_trace_marker::trace_header_event_trace_flag)); // header_flags

    set_at(buffer, 4, event_type);      // trace_class.type
    set_at(buffer, 5, std::uint8_t(0)); // trace_class.level
    set_at(buffer, 6, event_version);   // trace_class.version

    set_at(buffer, 16, timestamp); // system_time

    set_at(buffer, 24, provider_guid);

    return etl::parser::full_header_trace_header_view(buffer);
}

// NOTE: The events we create for the tests DO NOT HAVE complete information
//       set on them. Only what is currently read by the tested code is set.

void push_process_event(const etl::etl_file::header_data& file_header,
                        etl::event_observer&              observer,
                        std::span<std::byte>              buffer,
                        std::uint64_t                     timestamp,
                        std::uint32_t                     process_id,
                        std::string_view                  image_filename,
                        std::u16string_view               command_line,
                        bool                              load)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 8, process_id);
    set_at(event_data, 24 + 2 * file_header.pointer_size, image_filename);
    set_at(event_data, 24 + 2 * file_header.pointer_size + image_filename.size() + 1, command_line);
    const auto event_data_size = 24 + 2 * file_header.pointer_size + image_filename.size() + 1 + command_line.size() * 2 + 2 + 4;

    assert(etl::parser::process_v4_type_group1_event_view(event_data, file_header.pointer_size).process_id() == process_id);
    assert(etl::parser::process_v4_type_group1_event_view(event_data, file_header.pointer_size).image_filename() == image_filename);
    assert(etl::parser::process_v4_type_group1_event_view(event_data, file_header.pointer_size).command_line() == command_line);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::process_v4_type_group1_event_view::event_version,
                                                       etl::parser::event_trace_group::process,
                                                       load ? 1 : 2);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_thread_event(const etl::etl_file::header_data& file_header,
                       etl::event_observer&              observer,
                       std::span<std::byte>              buffer,
                       std::uint64_t                     timestamp,
                       std::uint32_t                     process_id,
                       std::uint32_t                     thread_id,
                       bool                              load)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 0, process_id);
    set_at(event_data, 4, thread_id);
    const auto event_data_size = 15 + 7 * file_header.pointer_size;

    assert(etl::parser::thread_v3_type_group1_event_view(event_data, file_header.pointer_size).process_id() == process_id);
    assert(etl::parser::thread_v3_type_group1_event_view(event_data, file_header.pointer_size).thread_id() == thread_id);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::thread_v3_type_group1_event_view::event_version,
                                                       etl::parser::event_trace_group::thread,
                                                       load ? 1 : 2);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_image_event(const etl::etl_file::header_data& file_header,
                      etl::event_observer&              observer,
                      std::span<std::byte>              buffer,
                      std::uint64_t                     timestamp,
                      std::uint32_t                     process_id,
                      std::uint64_t                     image_base,
                      std::uint64_t                     image_size,
                      std::uint32_t                     image_checksum,
                      std::u16string_view               image_filename,
                      bool                              start)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 0, image_base);
    set_at(event_data, file_header.pointer_size, image_size);
    set_at(event_data, 2 * file_header.pointer_size, process_id);
    set_at(event_data, 4 + 2 * file_header.pointer_size, image_checksum);
    set_at(event_data, 32 + 3 * file_header.pointer_size, image_filename);
    const auto event_data_size = 32 + 3 * file_header.pointer_size + image_filename.size() * 2 + 2;

    assert(etl::parser::image_v3_load_event_view(event_data, file_header.pointer_size).image_base() == image_base);
    assert(etl::parser::image_v3_load_event_view(event_data, file_header.pointer_size).image_size() == image_size);
    assert(etl::parser::image_v3_load_event_view(event_data, file_header.pointer_size).process_id() == process_id);
    assert(etl::parser::image_v3_load_event_view(event_data, file_header.pointer_size).image_checksum() == image_checksum);
    assert(etl::parser::image_v3_load_event_view(event_data, file_header.pointer_size).file_name() == image_filename);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::image_v3_load_event_view::event_version,
                                                       start ? etl::parser::event_trace_group::image : etl::parser::event_trace_group::process,
                                                       start ? 3 : 10);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_pdb_info_event(const etl::etl_file::header_data& file_header,
                         etl::event_observer&              observer,
                         std::span<std::byte>              buffer,
                         std::uint64_t                     timestamp,
                         std::uint32_t                     process_id,
                         std::uint64_t                     image_base,
                         common::guid                      guid,
                         std::uint32_t                     age,
                         std::string_view                  pdb_file_name)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 0, image_base);
    set_at(event_data, file_header.pointer_size, process_id);
    set_at(event_data, 4 + file_header.pointer_size, guid);
    set_at(event_data, 20 + file_header.pointer_size, age);
    set_at(event_data, 24 + file_header.pointer_size, pdb_file_name);
    const auto event_data_size = 24 + file_header.pointer_size + pdb_file_name.size() + 1;

    assert(etl::parser::image_id_v2_dbg_id_pdb_info_event_view(event_data, file_header.pointer_size).image_base() == image_base);
    assert(etl::parser::image_id_v2_dbg_id_pdb_info_event_view(event_data, file_header.pointer_size).process_id() == process_id);
    assert(etl::parser::image_id_v2_dbg_id_pdb_info_event_view(event_data, file_header.pointer_size).guid().instantiate() == guid);
    assert(etl::parser::image_id_v2_dbg_id_pdb_info_event_view(event_data, file_header.pointer_size).age() == age);
    assert(etl::parser::image_id_v2_dbg_id_pdb_info_event_view(event_data, file_header.pointer_size).pdb_file_name() == pdb_file_name);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::image_id_v2_dbg_id_pdb_info_event_view::event_version,
                                                     etl::parser::image_id_guid,
                                                     36);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_physical_disk_event(const etl::etl_file::header_data& file_header,
                              etl::event_observer&              observer,
                              std::span<std::byte>              buffer,
                              std::uint64_t                     timestamp,
                              std::uint32_t                     disk_number,
                              std::uint32_t                     partition_count)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 0, disk_number);
    set_at(event_data, 552, partition_count);
    const auto event_data_size = etl::parser::system_config_v2_physical_disk_event_view::static_size;

    assert(etl::parser::system_config_v2_physical_disk_event_view(event_data, file_header.pointer_size).disk_number() == disk_number);
    assert(etl::parser::system_config_v2_physical_disk_event_view(event_data, file_header.pointer_size).partition_count() == partition_count);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::system_config_v2_physical_disk_event_view::event_version,
                                                       etl::parser::event_trace_group::config,
                                                       11);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_logical_disk_event(const etl::etl_file::header_data& file_header,
                             etl::event_observer&              observer,
                             std::span<std::byte>              buffer,
                             std::uint64_t                     timestamp,
                             std::uint32_t                     disk_number,
                             std::uint32_t                     partition_number,
                             std::u16string_view               drive_letter)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 16, disk_number);
    set_at(event_data, 24, std::to_underlying(etl::parser::drive_type::partition));
    set_at(event_data, 28, drive_letter);
    set_at(event_data, 40, partition_number);
    const auto event_data_size = etl::parser::system_config_v2_logical_disk_event_view::static_size;

    assert(etl::parser::system_config_v2_logical_disk_event_view(event_data, file_header.pointer_size).disk_number() == disk_number);
    assert(etl::parser::system_config_v2_logical_disk_event_view(event_data, file_header.pointer_size).partition_number() == partition_number);
    assert(etl::parser::system_config_v2_logical_disk_event_view(event_data, file_header.pointer_size).drive_letter() == drive_letter);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::system_config_v2_logical_disk_event_view::event_version,
                                                       etl::parser::event_trace_group::config,
                                                       12);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_cpu_event(const etl::etl_file::header_data& file_header,
                    etl::event_observer&              observer,
                    std::span<std::byte>              buffer,
                    std::uint64_t                     timestamp,
                    std::u16string_view               computer_name,
                    std::uint16_t                     processor_architecture)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 20, computer_name);
    set_at(event_data, 800 + 2 * file_header.pointer_size, processor_architecture);
    const auto event_data_size = 812 + 2 * file_header.pointer_size;

    assert(etl::parser::system_config_v3_cpu_event_view(event_data, file_header.pointer_size).computer_name() == computer_name);
    assert(etl::parser::system_config_v3_cpu_event_view(event_data, file_header.pointer_size).processor_architecture() == processor_architecture);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::system_config_v3_cpu_event_view::event_version,
                                                       etl::parser::event_trace_group::config,
                                                       10);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_pnp_event(const etl::etl_file::header_data& file_header,
                    etl::event_observer&              observer,
                    std::span<std::byte>              buffer,
                    std::uint64_t                     timestamp,
                    std::u16string_view               device_description,
                    std::u16string_view               friendly_name)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::system_trace_header_view::static_size);

    set_at(event_data, 34, device_description);
    set_at(event_data, 34 + device_description.size() * 2 + 2, friendly_name);
    const auto event_data_size = 34 + device_description.size() * 2 + 2 + friendly_name.size() * 2 + 2 + 4;

    assert(etl::parser::system_config_v5_pnp_event_view(event_data, file_header.pointer_size).device_description() == device_description);
    assert(etl::parser::system_config_v5_pnp_event_view(event_data, file_header.pointer_size).friendly_name() == friendly_name);

    const auto event_header = make_system_trace_header(file_header, buffer, timestamp, event_data_size,
                                                       etl::parser::system_config_v5_pnp_event_view::event_version,
                                                       etl::parser::event_trace_group::config,
                                                       22);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_volume_mapping_event(const etl::etl_file::header_data& file_header,
                               etl::event_observer&              observer,
                               std::span<std::byte>              buffer,
                               std::uint64_t                     timestamp,
                               std::u16string_view               nt_path,
                               std::u16string_view               dos_path)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 0, nt_path);
    set_at(event_data, nt_path.size() * 2 + 2, dos_path);
    const auto event_data_size = (nt_path.size() + dos_path.size() + 2) * 2;

    assert(etl::parser::system_config_ex_v0_volume_mapping_event_view(event_data, file_header.pointer_size).nt_path() == nt_path);
    assert(etl::parser::system_config_ex_v0_volume_mapping_event_view(event_data, file_header.pointer_size).dos_path() == dos_path);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::system_config_ex_v0_volume_mapping_event_view::event_version,
                                                     etl::parser::system_config_ex_guid,
                                                     35);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_system_paths_event(const etl::etl_file::header_data& file_header,
                             etl::event_observer&              observer,
                             std::span<std::byte>              buffer,
                             std::uint64_t                     timestamp,
                             std::u16string_view               system_directory,
                             std::u16string_view               system_windows_directory)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 0, system_directory);
    set_at(event_data, system_directory.size() * 2 + 2, system_windows_directory);
    const auto event_data_size = (system_directory.size() + system_windows_directory.size() + 2) * 2;

    assert(etl::parser::system_config_ex_v0_system_paths_event_view(event_data, file_header.pointer_size).system_directory() == system_directory);
    assert(etl::parser::system_config_ex_v0_system_paths_event_view(event_data, file_header.pointer_size).system_windows_directory() == system_windows_directory);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::system_config_ex_v0_system_paths_event_view::event_version,
                                                     etl::parser::system_config_ex_guid,
                                                     33);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_build_info_event(const etl::etl_file::header_data& file_header,
                           etl::event_observer&              observer,
                           std::span<std::byte>              buffer,
                           std::uint64_t                     timestamp,
                           std::u16string_view               product_name)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 10, product_name);
    const auto event_data_size = 10 + product_name.size() * 2 + 2;

    assert(etl::parser::system_config_ex_v0_build_info_event_view(event_data, file_header.pointer_size).product_name() == product_name);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::system_config_ex_v0_build_info_event_view::event_version,
                                                     etl::parser::system_config_ex_guid,
                                                     32);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_sample_interval_event(const etl::etl_file::header_data& file_header,
                                etl::event_observer&              observer,
                                std::span<std::byte>              buffer,
                                std::uint64_t                     timestamp,
                                std::uint32_t                     source,
                                std::u16string_view               source_name,
                                bool                              is_start)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::perfinfo_trace_header_view::static_size);

    set_at(event_data, 0, source);
    set_at(event_data, 12, source_name);
    const auto event_data_size = 12 + source_name.size() * 2 + 2;

    assert(etl::parser::perfinfo_v3_sampled_profile_interval_event_view(event_data, file_header.pointer_size).source() == source);
    assert(etl::parser::perfinfo_v3_sampled_profile_interval_event_view(event_data, file_header.pointer_size).source_name() == source_name);

    const auto event_header = make_perfinfo_trace_header(file_header, buffer, timestamp, event_data_size,
                                                         etl::parser::perfinfo_v3_sampled_profile_interval_event_view::event_version,
                                                         etl::parser::event_trace_group::perfinfo,
                                                         is_start ? 73 : 74);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_sample_event(const etl::etl_file::header_data& file_header,
                       etl::event_observer&              observer,
                       std::span<std::byte>              buffer,
                       std::uint64_t                     timestamp,
                       std::uint32_t                     thread_id,
                       std::uint64_t                     instruction_pointer)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::perfinfo_trace_header_view::static_size);

    set_at(event_data, 0, instruction_pointer);
    set_at(event_data, file_header.pointer_size, thread_id);
    const auto event_data_size = 8 + file_header.pointer_size;

    assert(etl::parser::perfinfo_v2_sampled_profile_event_view(event_data, file_header.pointer_size).instruction_pointer() == instruction_pointer);
    assert(etl::parser::perfinfo_v2_sampled_profile_event_view(event_data, file_header.pointer_size).thread_id() == thread_id);

    const auto event_header = make_perfinfo_trace_header(file_header, buffer, timestamp, event_data_size,
                                                         etl::parser::perfinfo_v2_sampled_profile_event_view::event_version,
                                                         etl::parser::event_trace_group::perfinfo,
                                                         46);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_pmc_sample_event(const etl::etl_file::header_data& file_header,
                           etl::event_observer&              observer,
                           std::span<std::byte>              buffer,
                           std::uint64_t                     timestamp,
                           std::uint32_t                     thread_id,
                           std::uint16_t                     source,
                           std::uint64_t                     instruction_pointer)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::perfinfo_trace_header_view::static_size);

    set_at(event_data, 0, instruction_pointer);
    set_at(event_data, file_header.pointer_size, thread_id);
    set_at(event_data, file_header.pointer_size + 4, source);
    const auto event_data_size = 8 + file_header.pointer_size;

    assert(etl::parser::perfinfo_v2_pmc_counter_profile_event_view(event_data, file_header.pointer_size).instruction_pointer() == instruction_pointer);
    assert(etl::parser::perfinfo_v2_pmc_counter_profile_event_view(event_data, file_header.pointer_size).thread_id() == thread_id);

    const auto event_header = make_perfinfo_trace_header(file_header, buffer, timestamp, event_data_size,
                                                         etl::parser::perfinfo_v2_pmc_counter_profile_event_view::event_version,
                                                         etl::parser::event_trace_group::perfinfo,
                                                         47);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_stack_event(const etl::etl_file::header_data& file_header,
                      etl::event_observer&              observer,
                      std::span<std::byte>              buffer,
                      std::uint64_t                     timestamp,
                      std::uint32_t                     thread_id,
                      std::uint64_t                     event_timestamp,
                      std::vector<std::uint64_t>        stack)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::perfinfo_trace_header_view::static_size);

    set_at(event_data, 0, event_timestamp);
    set_at(event_data, 12, thread_id);
    for(std::size_t i = 0; i < stack.size(); ++i)
    {
        set_at(event_data, 16 + i * file_header.pointer_size, stack[i]);
    }
    const auto event_data_size = 16 + stack.size() * file_header.pointer_size;

    assert(etl::parser::stackwalk_v2_stack_event_view(event_data, file_header.pointer_size).event_timestamp() == event_timestamp);
    assert(etl::parser::stackwalk_v2_stack_event_view(event_data, file_header.pointer_size).thread_id() == thread_id);

    const auto event_header = make_perfinfo_trace_header(file_header, buffer, timestamp, event_data_size,
                                                         etl::parser::stackwalk_v2_stack_event_view::event_version,
                                                         etl::parser::event_trace_group::stackwalk,
                                                         32);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_prof_started_event(const etl::etl_file::header_data& file_header,
                             etl::event_observer&              observer,
                             std::span<std::byte>              buffer,
                             std::uint64_t                     timestamp,
                             std::uint32_t                     process_id)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 0, process_id);
    const auto event_data_size = 16;

    assert(etl::parser::vs_diagnostics_hub_target_profiling_started_event_view(event_data, file_header.pointer_size).process_id() == process_id);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::vs_diagnostics_hub_target_profiling_started_event_view::event_version,
                                                     etl::parser::vs_diagnostics_hub_guid,
                                                     1);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

void push_snail_prof_target_event(const etl::etl_file::header_data& file_header,
                                  etl::event_observer&              observer,
                                  std::span<std::byte>              buffer,
                                  std::uint64_t                     timestamp,
                                  std::uint32_t                     process_id)
{
    std::ranges::fill(buffer, std::byte{});

    const auto event_data = buffer.subspan(etl::parser::full_header_trace_header_view::static_size);

    set_at(event_data, 0, process_id);
    const auto event_data_size = 4;

    assert(etl::parser::snail_profiler_profile_target_event_view(event_data, file_header.pointer_size).process_id() == process_id);

    const auto event_header = make_full_trace_header(file_header, buffer, timestamp, event_data_size,
                                                     etl::parser::snail_profiler_profile_target_event_view::event_version,
                                                     etl::parser::snail_profiler_guid,
                                                     1);

    observer.handle(file_header, event_header, event_data.subspan(0, event_data_size));
}

// we use the same file_header for all tests (except for the pointer size, it doesn't really matter anyways)
static const etl::etl_file::header_data file_header = {
    .start_time           = common::nt_sys_time(common::nt_duration(100)),
    .end_time             = common::nt_sys_time(common::nt_duration(200)),
    .start_time_qpc_ticks = 1000,
    .qpc_frequency        = 10,
    .pointer_size         = 8,
    .number_of_processors = 12,
    .number_of_buffers    = 42};

} // namespace

TEST(EtlFileProcessContext, Processes)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // Start a process, but do not end it
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       10, 123, "\\Device\\HarddiskVolume1\\my\\path\\to\\a.exe", u"a.exe arg1 arg2", true);

    // Start at process that will be ended later
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       15, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\b.exe", u"b.exe", true);

    // End a process, that has never been started (just make sure we are fault tolerant)
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       17, 789, "\\Device\\HarddiskVolume1\\my\\path\\to\\c.exe", u"c.exe", false);

    // End the process that we have started second
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       20, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\b.exe", u"b.exe", false);

    // Start and end a process with the same process id that was already running and has ended
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       35, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\d.exe", u"d.exe", true);
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       45, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\d.exe", u"d.exe", false);

    context.finish();

    EXPECT_EQ(context.profiler_processes().size(), 0);
    EXPECT_EQ(context.get_threads().size(), 0);

    const auto& processes = context.get_processes().all_entries();
    EXPECT_EQ(processes.size(), 2);

    ASSERT_TRUE(processes.contains(123));
    ASSERT_EQ(processes.at(123).size(), 1);

    const auto process_a = processes.at(123)[0];
    EXPECT_EQ(process_a.id, 123);
    EXPECT_EQ(process_a.timestamp, 10);
    EXPECT_EQ(process_a.payload.image_filename, "\\Device\\HarddiskVolume1\\my\\path\\to\\a.exe");
    EXPECT_EQ(process_a.payload.command_line, u"a.exe arg1 arg2");
    EXPECT_EQ(process_a.payload.end_time, std::nullopt);

    ASSERT_TRUE(processes.contains(456));
    ASSERT_EQ(processes.at(456).size(), 2);

    const auto process_b = processes.at(456)[0];
    EXPECT_EQ(process_b.id, 456);
    EXPECT_EQ(process_b.timestamp, 15);
    EXPECT_EQ(process_b.payload.image_filename, "\\Device\\HarddiskVolume1\\my\\path\\to\\b.exe");
    EXPECT_EQ(process_b.payload.command_line, u"b.exe");
    EXPECT_EQ(process_b.payload.end_time, 20);

    const auto process_d = processes.at(456)[1];
    EXPECT_EQ(process_d.id, 456);
    EXPECT_EQ(process_d.timestamp, 35);
    EXPECT_EQ(process_d.payload.image_filename, "\\Device\\HarddiskVolume1\\my\\path\\to\\d.exe");
    EXPECT_EQ(process_d.payload.command_line, u"d.exe");
    EXPECT_EQ(process_d.payload.end_time, 45);
}

TEST(EtlFileProcessContext, Threads)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // Start a first process
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       10, 123, "\\Device\\HarddiskVolume1\\my\\path\\to\\a.exe", u"a.exe arg1 arg2", true);

    // Start a thread
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);
    // handle duplicate start events
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);

    // Start another process
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       15, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\b.exe", u"b.exe", true);

    // Start and end a thread
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      15, 456, 222, true);
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      16, 456, 222, false);

    // // Start another thread in the process with the same thread id
    // push_thread_event(file_header, context.observer(), writable_bytes_buffer,
    //                   20, 456, 222, true);

    // End the second process and start a new one with the same PID
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       25, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\b.exe", u"b.exe", false);
    push_process_event(file_header, context.observer(), writable_bytes_buffer,
                       35, 456, "\\Device\\HarddiskVolume1\\my\\path\\to\\d.exe", u"d.exe", true);

    // Start a new thread for the second process with PID 456
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      40, 456, 333, true);

    // Start a new thread with a TID that has been used already, but the process has ended and we start it
    // for another process.
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      45, 123, 222, true);

    // End a thread that has never been started (fault tolerance)
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      50, 123, 444, false);

    context.finish();

    const auto& threads = context.get_threads().all_entries();
    EXPECT_EQ(threads.size(), 3);

    ASSERT_TRUE(threads.contains(111));
    ASSERT_EQ(threads.at(111).size(), 1);

    const auto thread_111 = threads.at(111)[0];
    EXPECT_EQ(thread_111.id, 111);
    EXPECT_EQ(thread_111.timestamp, 10);
    EXPECT_EQ(thread_111.payload.process_id, 123);
    EXPECT_EQ(thread_111.payload.end_time, std::nullopt);

    ASSERT_TRUE(threads.contains(222));
    ASSERT_EQ(threads.at(222).size(), 2);

    const auto thread_222_1 = threads.at(222)[0];
    EXPECT_EQ(thread_222_1.id, 222);
    EXPECT_EQ(thread_222_1.timestamp, 15);
    EXPECT_EQ(thread_222_1.payload.process_id, 456);
    EXPECT_EQ(thread_222_1.payload.end_time, 16);

    const auto thread_222_2 = threads.at(222)[1];
    EXPECT_EQ(thread_222_2.id, 222);
    EXPECT_EQ(thread_222_2.timestamp, 45);
    EXPECT_EQ(thread_222_2.payload.process_id, 123);
    EXPECT_EQ(thread_222_2.payload.end_time, std::nullopt);

    ASSERT_TRUE(threads.contains(333));
    ASSERT_EQ(threads.at(333).size(), 1);

    const auto thread_333 = threads.at(333)[0];
    EXPECT_EQ(thread_333.id, 333);
    EXPECT_EQ(thread_333.timestamp, 40);
    EXPECT_EQ(thread_333.payload.process_id, 456);
    EXPECT_EQ(thread_333.payload.end_time, std::nullopt);

    const auto& processes = context.get_processes().all_entries();
    const auto  process_a = processes.at(123)[0];
    const auto  process_b = processes.at(456)[0];
    const auto  process_c = processes.at(456)[1];

    using threads_set = std::set<analysis::unique_thread_id>;

    EXPECT_EQ(context.get_process_threads(*process_a.payload.unique_id), (threads_set{*thread_111.payload.unique_id, *thread_222_2.payload.unique_id}));

    EXPECT_EQ(context.get_process_threads(*process_b.payload.unique_id), (threads_set{*thread_222_1.payload.unique_id}));

    EXPECT_EQ(context.get_process_threads(*process_c.payload.unique_id), (threads_set{*thread_333.payload.unique_id}));
}

TEST(EtlFileProcessContext, Images)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     10, 123, 0xAABB, 1000, 4567, u"\\Device\\HarddiskVolume2\\a.dll", false);

    const auto b_guid = common::guid{
        0xc26e'6ef3, 0x76ab, 0x4f0d, {0xa7, 0x98, 0x63, 0x3e, 0x15, 0xb6, 0x2, 0x1d}
    };
    push_pdb_info_event(file_header, context.observer(), writable_bytes_buffer,
                        12, 456, 0xCCDD, b_guid, 1, "b.pdb");
    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     12, 456, 0xCCDD, 2000, 8910, u"\\Device\\HarddiskVolume2\\b.dll", false);

    const auto c_guid = common::guid{
        0x93f2'0298, 0x6828, 0x4dce, {0x90, 0xf2, 0x80, 0x4d, 0x99, 0x66, 0x5, 0x1a}
    };
    push_pdb_info_event(file_header, context.observer(), writable_bytes_buffer,
                        15, 789, 0xEEFF, c_guid, 3, "c.pdb");
    const auto d_guid = common::guid{
        0x93f2'0298, 0x6828, 0x4dce, {0x90, 0xf2, 0x80, 0x4d, 0x99, 0x66, 0x5, 0x1a}
    };
    push_pdb_info_event(file_header, context.observer(), writable_bytes_buffer,
                        15, 789, 0x77FF, d_guid, 5, "d.pdb");
    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 789, 0xEEFF, 500, 1112, u"\\Device\\HarddiskVolume2\\c.dll", false);
    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 789, 0x77FF, 300, 1314, u"\\Device\\HarddiskVolume2\\d.dll", false);

    context.finish();

    const auto& modules_123 = context.get_modules(123);
    EXPECT_EQ(modules_123.all_modules().size(), 1);

    const auto& module_a = modules_123.all_modules().at(0);
    EXPECT_EQ(module_a.base, 0xAABB);
    EXPECT_EQ(module_a.size, 1000);
    EXPECT_EQ(module_a.payload.checksum, 4567);
    EXPECT_EQ(module_a.payload.filename, "\\Device\\HarddiskVolume2\\a.dll");
    EXPECT_EQ(module_a.payload.pdb_info, std::nullopt);

    const auto& modules_456 = context.get_modules(456);
    EXPECT_EQ(modules_456.all_modules().size(), 1);

    const auto& module_b = modules_456.all_modules().at(0);
    EXPECT_EQ(module_b.base, 0xCCDD);
    EXPECT_EQ(module_b.size, 2000);
    EXPECT_EQ(module_b.payload.checksum, 8910);
    EXPECT_EQ(module_b.payload.filename, "\\Device\\HarddiskVolume2\\b.dll");
    EXPECT_EQ(module_b.payload.pdb_info, (analysis::detail::pdb_info{
                                             .pdb_name = "b.pdb",
                                             .guid     = b_guid,
                                             .age      = 1}));

    const auto& modules_789 = context.get_modules(789);
    EXPECT_EQ(modules_789.all_modules().size(), 2);

    const auto& module_c = modules_789.all_modules().at(0);
    EXPECT_EQ(module_c.base, 0xEEFF);
    EXPECT_EQ(module_c.size, 500);
    EXPECT_EQ(module_c.payload.checksum, 1112);
    EXPECT_EQ(module_c.payload.filename, "\\Device\\HarddiskVolume2\\c.dll");
    EXPECT_EQ(module_c.payload.pdb_info, (analysis::detail::pdb_info{
                                             .pdb_name = "c.pdb",
                                             .guid     = c_guid,
                                             .age      = 3}));

    const auto& module_d = modules_789.all_modules().at(1);
    EXPECT_EQ(module_d.base, 0x77FF);
    EXPECT_EQ(module_d.size, 300);
    EXPECT_EQ(module_d.payload.checksum, 1314);
    EXPECT_EQ(module_d.payload.filename, "\\Device\\HarddiskVolume2\\d.dll");
    EXPECT_EQ(module_d.payload.pdb_info, (analysis::detail::pdb_info{
                                             .pdb_name = "d.pdb",
                                             .guid     = d_guid,
                                             .age      = 5}));
}

TEST(EtlFileProcessContext, VolumeMappingNoXPerf)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     5, 123, 0xAABB, 1000, 4567, u"\\Device\\HarddiskVolume1\\a.dll", true);

    push_physical_disk_event(file_header, context.observer(), writable_bytes_buffer,
                             10, 1, 1);
    push_physical_disk_event(file_header, context.observer(), writable_bytes_buffer,
                             10, 2, 2);
    push_logical_disk_event(file_header, context.observer(), writable_bytes_buffer,
                            10, 1, 1, u"C:");
    push_logical_disk_event(file_header, context.observer(), writable_bytes_buffer,
                            10, 2, 1, u"E:");
    push_logical_disk_event(file_header, context.observer(), writable_bytes_buffer,
                            10, 2, 2, u"F:");

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     12, 456, 0xCCDD, 2000, 8910, u"\\Device\\HarddiskVolume2\\b.dll", false);

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 789, 0xEEFF, 500, 1112, u"\\SystemRoot\\System32\\ntdll.dll", false);

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 789, 0x77FF, 300, 1314, u"\\Device\\HarddiskVolume3\\d.dll", false);

    context.finish();

    const auto& modules_123 = context.get_modules(123);
    EXPECT_EQ(modules_123.all_modules().size(), 1);

    const auto& module_a = modules_123.all_modules().at(0);
    EXPECT_EQ(module_a.base, 0xAABB);
    EXPECT_EQ(module_a.size, 1000);
    EXPECT_EQ(module_a.payload.checksum, 4567);
    EXPECT_EQ(module_a.payload.filename, "C:\\a.dll");
    EXPECT_EQ(module_a.payload.pdb_info, std::nullopt);

    const auto& modules_456 = context.get_modules(456);
    EXPECT_EQ(modules_456.all_modules().size(), 1);

    const auto& module_b = modules_456.all_modules().at(0);
    EXPECT_EQ(module_b.base, 0xCCDD);
    EXPECT_EQ(module_b.size, 2000);
    EXPECT_EQ(module_b.payload.checksum, 8910);
    EXPECT_EQ(module_b.payload.filename, "E:\\b.dll");
    EXPECT_EQ(module_b.payload.pdb_info, std::nullopt);

    const auto& modules_789 = context.get_modules(789);
    EXPECT_EQ(modules_789.all_modules().size(), 2);

    const auto& module_c = modules_789.all_modules().at(0);
    EXPECT_EQ(module_c.base, 0xEEFF);
    EXPECT_EQ(module_c.size, 500);
    EXPECT_EQ(module_c.payload.checksum, 1112);
    EXPECT_EQ(module_c.payload.filename, "C:\\WINDOWS\\System32\\ntdll.dll");
    EXPECT_EQ(module_c.payload.pdb_info, std::nullopt);

    const auto& module_d = modules_789.all_modules().at(1);
    EXPECT_EQ(module_d.base, 0x77FF);
    EXPECT_EQ(module_d.size, 300);
    EXPECT_EQ(module_d.payload.checksum, 1314);
    EXPECT_EQ(module_d.payload.filename, "F:\\d.dll");
    EXPECT_EQ(module_d.payload.pdb_info, std::nullopt);
}

TEST(EtlFileProcessContext, VolumeMappingXPerf)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_volume_mapping_event(file_header, context.observer(), writable_bytes_buffer,
                              0, u"\\Device\\HarddiskVolume1", u"C:");
    push_volume_mapping_event(file_header, context.observer(), writable_bytes_buffer,
                              0, u"\\Device\\HarddiskVolume2", u"D:");

    push_system_paths_event(file_header, context.observer(), writable_bytes_buffer,
                            0, u"C:\\WINDOWS\\System32\\", u"C:\\WINDOWS\\");

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     5, 123, 0xAABB, 1000, 4567, u"\\Device\\HarddiskVolume1\\a.dll", true);

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     12, 456, 0xCCDD, 2000, 8910, u"\\Device\\HarddiskVolume2\\b.dll", false);

    push_image_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 789, 0xEEFF, 500, 1112, u"\\SystemRoot\\System32\\ntdll.dll", false);

    context.finish();

    const auto& modules_123 = context.get_modules(123);
    EXPECT_EQ(modules_123.all_modules().size(), 1);

    const auto& module_a = modules_123.all_modules().at(0);
    EXPECT_EQ(module_a.base, 0xAABB);
    EXPECT_EQ(module_a.size, 1000);
    EXPECT_EQ(module_a.payload.checksum, 4567);
    EXPECT_EQ(module_a.payload.filename, "C:\\a.dll");
    EXPECT_EQ(module_a.payload.pdb_info, std::nullopt);

    const auto& modules_456 = context.get_modules(456);
    EXPECT_EQ(modules_456.all_modules().size(), 1);

    const auto& module_b = modules_456.all_modules().at(0);
    EXPECT_EQ(module_b.base, 0xCCDD);
    EXPECT_EQ(module_b.size, 2000);
    EXPECT_EQ(module_b.payload.checksum, 8910);
    EXPECT_EQ(module_b.payload.filename, "D:\\b.dll");
    EXPECT_EQ(module_b.payload.pdb_info, std::nullopt);

    const auto& modules_789 = context.get_modules(789);
    EXPECT_EQ(modules_789.all_modules().size(), 1);

    const auto& module_c = modules_789.all_modules().at(0);
    EXPECT_EQ(module_c.base, 0xEEFF);
    EXPECT_EQ(module_c.size, 500);
    EXPECT_EQ(module_c.payload.checksum, 1112);
    EXPECT_EQ(module_c.payload.filename, "C:\\WINDOWS\\System32\\ntdll.dll");
    EXPECT_EQ(module_c.payload.pdb_info, std::nullopt);
}

TEST(EtlFileProcessContext, NoSamples)
{
    etl_file_process_context context;

    context.finish();

    const auto sample_sources = context.sample_source_names();
    EXPECT_EQ(sample_sources.size(), 0);

    EXPECT_EQ(context.thread_samples(111, 10, 15, 0).size(), 0);    // empty for regular samples
    EXPECT_EQ(context.thread_samples(111, 10, 15, 1234).size(), 0); // empty for random pmc samples
}

TEST(EtlFileProcessContext, Samples)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // Fault tolerant: The first sample comes in before the thread it belongs to
    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 111, 0x1'234A);

    // Now configure the sample interval
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               10, 0, u"Timer", true);

    // Now the thread is started (in this test we do not care about the process)
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);

    // Another sample for this thread
    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      12, 111, 0x1'234B);

    // The thread ends...
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      15, 123, 111, false);
    // And another one starts under the same thread-ID
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      20, 123, 111, true);

    // Now there is a sample for the second thread
    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      22, 111, 0x1'234C);

    // And terminate the sampling
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               35, 0, u"Timer", false);

    context.finish();

    const auto& sample_sources = context.sample_source_names();
    ASSERT_EQ(sample_sources.size(), 1);
    ASSERT_TRUE(sample_sources.contains(0));
    const auto sample_source_name = sample_sources.at(0);
    EXPECT_EQ(sample_source_name, std::u16string(u"Timer"));

    // Check samples for the first thread
    const auto thread_1_samples = context.thread_samples(111, 10, 15, 0);
    EXPECT_EQ(thread_1_samples.size(), 2);
    EXPECT_EQ(thread_1_samples[0].timestamp, 10);
    EXPECT_EQ(thread_1_samples[0].thread_id, 111);
    EXPECT_EQ(thread_1_samples[0].instruction_pointer, 0x1'234A);
    EXPECT_EQ(thread_1_samples[1].timestamp, 12);
    EXPECT_EQ(thread_1_samples[1].thread_id, 111);
    EXPECT_EQ(thread_1_samples[1].instruction_pointer, 0x1'234B);

    // Check samples for the second thread
    const auto thread_2_samples = context.thread_samples(111, 20, std::nullopt, 0);
    EXPECT_EQ(thread_2_samples.size(), 1);
    EXPECT_EQ(thread_2_samples[0].timestamp, 22);
    EXPECT_EQ(thread_2_samples[0].thread_id, 111);
    EXPECT_EQ(thread_2_samples[0].instruction_pointer, 0x1'234C);

    // Check samples for non existing threads
    EXPECT_EQ(context.thread_samples(222, 0, std::nullopt, 0).size(), 0);

    // Check samples out of range
    EXPECT_EQ(context.thread_samples(111, 100, std::nullopt, 0).size(), 0);
    EXPECT_EQ(context.thread_samples(111, 0, 5, 0).size(), 0);

    // Check samples for non existing sample source id
    EXPECT_EQ(context.thread_samples(111, 10, 15, 1234).size(), 0);
}

TEST(EtlFileProcessContext, PmcSamples)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // Fault tolerant: The first sample comes in before the thread it belongs to
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          10, 111, 29, 0x1'234A);

    // Now the thread is started (in this test we do not care about the process)
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);

    // Another sample for this thread
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          12, 111, 29, 0x1'234B);

    // The thread ends...
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      15, 123, 111, false);
    // And another one starts under the same thread-ID
    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      20, 123, 111, true);

    // Now there is a sample for the second thread
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          22, 111, 29, 0x1'234C);

    context.finish();

    const auto& sample_sources = context.sample_source_names();
    ASSERT_EQ(sample_sources.size(), 1);
    ASSERT_TRUE(sample_sources.contains(29));
    const auto sample_source_name = sample_sources.at(29);
    EXPECT_EQ(sample_source_name, std::u16string(u"Unknown (29)"));

    // Check samples for the first thread
    const auto thread_1_samples = context.thread_samples(111, 10, 15, 29);
    EXPECT_EQ(thread_1_samples.size(), 2);
    EXPECT_EQ(thread_1_samples[0].timestamp, 10);
    EXPECT_EQ(thread_1_samples[0].thread_id, 111);
    EXPECT_EQ(thread_1_samples[0].instruction_pointer, 0x1'234A);
    EXPECT_EQ(thread_1_samples[1].timestamp, 12);
    EXPECT_EQ(thread_1_samples[1].thread_id, 111);
    EXPECT_EQ(thread_1_samples[1].instruction_pointer, 0x1'234B);

    // Check samples for the second thread
    const auto thread_2_samples = context.thread_samples(111, 20, std::nullopt, 29);
    EXPECT_EQ(thread_2_samples.size(), 1);
    EXPECT_EQ(thread_2_samples[0].timestamp, 22);
    EXPECT_EQ(thread_2_samples[0].thread_id, 111);
    EXPECT_EQ(thread_2_samples[0].instruction_pointer, 0x1'234C);

    // Check samples for non existing threads
    EXPECT_EQ(context.thread_samples(222, 0, std::nullopt, 29).size(), 0);

    // Check samples out of range
    EXPECT_EQ(context.thread_samples(111, 100, std::nullopt, 29).size(), 0);
    EXPECT_EQ(context.thread_samples(111, 0, 5, 29).size(), 0);

    // Check samples for non existing sample source id
    EXPECT_EQ(context.thread_samples(111, 10, 15, 1234).size(), 0);
}

TEST(EtlFileProcessContext, Stacks)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);

    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 111, 0x1'234A);
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     11, 111, 10, {0x1'234B, 0x1'234C, 0x1'234D});
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     11, 111, 10, {0x1234'B000'0000'0000, 0x1234'C000'0000'0000, 0x1234'D000'0000'0000});

    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      20, 111, 0x5'678A);
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     20, 111, 20, {0x5'678B, 0x5'678C, 0x5'678D});

    // Fault tolerance: stack for non existing thread
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     20, 222, 20, {0xFFAA, 0xFFBB});
    // Fault tolerance: stack for non existing samples
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     6, 111, 5, {0xFFCC, 0xFFDD});
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 111, 15, {0xFFCC, 0xFFDD});
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     40, 111, 35, {0xFFCC, 0xFFDD});

    context.finish();

    const auto& sample_sources = context.sample_source_names();
    ASSERT_EQ(sample_sources.size(), 1);
    ASSERT_TRUE(sample_sources.contains(0));
    const auto sample_source_name = sample_sources.at(0);
    EXPECT_EQ(sample_source_name, std::u16string(u"Unknown (0)"));

    // Check samples for the first thread
    const auto thread_1_samples = context.thread_samples(111, 10, std::nullopt, 0);
    EXPECT_EQ(thread_1_samples.size(), 2);
    EXPECT_EQ(thread_1_samples[0].timestamp, 10);
    EXPECT_EQ(thread_1_samples[0].thread_id, 111);
    EXPECT_EQ(thread_1_samples[0].instruction_pointer, 0x1'234A);
    EXPECT_NE(thread_1_samples[0].user_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*thread_1_samples[0].user_mode_stack), (std::vector<std::uint64_t>{0x1'234B, 0x1'234C, 0x1'234D}));
    EXPECT_NE(thread_1_samples[0].kernel_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*thread_1_samples[0].kernel_mode_stack), (std::vector<std::uint64_t>{0x1234'B000'0000'0000, 0x1234'C000'0000'0000, 0x1234'D000'0000'0000}));

    EXPECT_EQ(thread_1_samples[1].timestamp, 20);
    EXPECT_EQ(thread_1_samples[1].thread_id, 111);
    EXPECT_EQ(thread_1_samples[1].instruction_pointer, 0x5'678A);
    EXPECT_NE(thread_1_samples[1].user_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*thread_1_samples[1].user_mode_stack), (std::vector<std::uint64_t>{0x5'678B, 0x5'678C, 0x5'678D}));
    EXPECT_EQ(thread_1_samples[1].kernel_mode_stack, std::nullopt);
}

TEST(EtlFileProcessContext, MixedSamplesStacks)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    // This tests a case where the regular profiler samples where enabled and additionally the "Timer"
    // PMC, which is basically exactly the same source.
    // And we additionally have some "BranchMispredictions" PMC samples.

    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               5, 0, u"Timer", true);
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               5, 0, u"Timer", true);
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               5, 11, u"BranchMispredictions", true);

    push_thread_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 123, 111, true);

    // First "Timer" sample is present two times.
    push_sample_event(file_header, context.observer(), writable_bytes_buffer,
                      10, 111, 0x1'234A);
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          10, 111, 0, 0x1'234A);
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     11, 111, 10, {0x1'234B, 0x1'234C, 0x1'234D});
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     11, 111, 10, {0x1234'B000'0000'0000, 0x1234'C000'0000'0000, 0x1234'D000'0000'0000});

    // Now an unrelated "BranchMispredictions" sample.
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          15, 111, 11, 0x5'678A);
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     15, 111, 15, {0x5'678B, 0x5'678C, 0x5'678D});

    // Second "Timer" sample is only present as PMC event.
    // This should usually not happen, but we test for it anyways. Additionally, this allows us to
    // distinguish the samples we retrieve later on.
    push_pmc_sample_event(file_header, context.observer(), writable_bytes_buffer,
                          20, 111, 0, 0x9'101A);
    push_stack_event(file_header, context.observer(), writable_bytes_buffer,
                     20, 111, 20, {0x9'101B, 0x9'101C, 0x9'101D});

    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               25, 0, u"Timer", false);
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               25, 0, u"Timer", false);
    push_sample_interval_event(file_header, context.observer(), writable_bytes_buffer,
                               25, 11, u"BranchMispredictions", false);

    context.finish();

    const auto& sample_sources = context.sample_source_names();
    ASSERT_EQ(sample_sources.size(), 2);
    ASSERT_TRUE(sample_sources.contains(0));
    const auto sample_source_name_1 = sample_sources.at(0);
    EXPECT_EQ(sample_source_name_1, std::u16string(u"Timer"));
    ASSERT_TRUE(sample_sources.contains(11));
    const auto sample_source_name_2 = sample_sources.at(11);
    EXPECT_EQ(sample_source_name_2, std::u16string(u"BranchMispredictions"));

    // Check samples for the "Timer" samples.
    // We expect to get the PMC samples when both are present.
    const auto samples_1 = context.thread_samples(111, 10, std::nullopt, 0);
    EXPECT_EQ(samples_1.size(), 2);
    EXPECT_EQ(samples_1[0].timestamp, 10);
    EXPECT_EQ(samples_1[0].thread_id, 111);
    EXPECT_EQ(samples_1[0].instruction_pointer, 0x1'234A);
    EXPECT_NE(samples_1[0].user_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*samples_1[0].user_mode_stack), (std::vector<std::uint64_t>{0x1'234B, 0x1'234C, 0x1'234D}));
    EXPECT_NE(samples_1[0].kernel_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*samples_1[0].kernel_mode_stack), (std::vector<std::uint64_t>{0x1234'B000'0000'0000, 0x1234'C000'0000'0000, 0x1234'D000'0000'0000}));

    EXPECT_EQ(samples_1[1].timestamp, 20);
    EXPECT_EQ(samples_1[1].thread_id, 111);
    EXPECT_EQ(samples_1[1].instruction_pointer, 0x9'101A);
    EXPECT_NE(samples_1[1].user_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*samples_1[1].user_mode_stack), (std::vector<std::uint64_t>{0x9'101B, 0x9'101C, 0x9'101D}));
    EXPECT_EQ(samples_1[1].kernel_mode_stack, std::nullopt);

    // Check samples for the "BranchMispredictions" samples.
    const auto samples_2 = context.thread_samples(111, 10, std::nullopt, 11);
    EXPECT_EQ(samples_2.size(), 1);

    EXPECT_EQ(samples_2[0].timestamp, 15);
    EXPECT_EQ(samples_2[0].thread_id, 111);
    EXPECT_EQ(samples_2[0].instruction_pointer, 0x5'678A);
    EXPECT_NE(samples_2[0].user_mode_stack, std::nullopt);
    EXPECT_EQ(context.stack(*samples_2[0].user_mode_stack), (std::vector<std::uint64_t>{0x5'678B, 0x5'678C, 0x5'678D}));
    EXPECT_EQ(samples_2[0].kernel_mode_stack, std::nullopt);
}

TEST(EtlFileProcessContext, SystemInfo)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    EXPECT_EQ(context.computer_name(), std::nullopt);
    EXPECT_EQ(context.processor_architecture(), std::nullopt);
    EXPECT_EQ(context.processor_name(), std::nullopt);
    EXPECT_EQ(context.os_name(), std::nullopt);

    push_cpu_event(file_header, context.observer(), writable_bytes_buffer,
                   0, u"my-👻-computer", 9);
    push_pnp_event(file_header, context.observer(), writable_bytes_buffer,
                   0, u"Intel Processor", u"Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    push_build_info_event(file_header, context.observer(), writable_bytes_buffer,
                          0, u"Windows 10");

    context.finish();

    ASSERT_NE(context.computer_name(), std::nullopt);
    ASSERT_NE(context.processor_architecture(), std::nullopt);
    ASSERT_NE(context.processor_name(), std::nullopt);
    ASSERT_NE(context.os_name(), std::nullopt);

    EXPECT_EQ(*context.computer_name(), u"my-👻-computer");
    EXPECT_EQ(*context.processor_architecture(), 9);
    EXPECT_EQ(*context.processor_name(), u"Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(*context.os_name(), u"Windows 10");
}

TEST(EtlFileProcessContext, VsDiagnosticsHub)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_prof_started_event(file_header, context.observer(), writable_bytes_buffer,
                            10, 123);
    push_prof_started_event(file_header, context.observer(), writable_bytes_buffer,
                            20, 456);

    context.finish();

    const auto& processes = context.profiler_processes();
    EXPECT_EQ(processes.size(), 2);

    const auto process_123 = processes.at(etl_file_process_context::process_key{123, 10});
    EXPECT_EQ(process_123.process_id, 123);
    EXPECT_EQ(process_123.start_timestamp, 10);

    const auto process_456 = processes.at(etl_file_process_context::process_key{456, 20});
    EXPECT_EQ(process_456.process_id, 456);
    EXPECT_EQ(process_456.start_timestamp, 20);
}

TEST(EtlFileProcessContext, SnailProfiler)
{
    etl_file_process_context context;

    std::array<std::uint8_t, 1024> buffer;
    const auto                     writable_bytes_buffer = std::as_writable_bytes(std::span(buffer));

    push_snail_prof_target_event(file_header, context.observer(), writable_bytes_buffer,
                                 10, 123);
    push_snail_prof_target_event(file_header, context.observer(), writable_bytes_buffer,
                                 20, 456);

    context.finish();

    const auto& processes = context.profiler_processes();
    EXPECT_EQ(processes.size(), 2);

    const auto process_123 = processes.at(etl_file_process_context::process_key{123, 10});
    EXPECT_EQ(process_123.process_id, 123);
    EXPECT_EQ(process_123.start_timestamp, 10);

    const auto process_456 = processes.at(etl_file_process_context::process_key{456, 20});
    EXPECT_EQ(process_456.process_id, 456);
    EXPECT_EQ(process_456.start_timestamp, 20);
}
