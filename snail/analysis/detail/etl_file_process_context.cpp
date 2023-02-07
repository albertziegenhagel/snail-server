
#include <snail/analysis/detail/etl_file_process_context.hpp>

#include <ranges>

#include <snail/etl/parser/records/kernel/config.hpp>
#include <snail/etl/parser/records/kernel/image.hpp>
#include <snail/etl/parser/records/kernel/perfinfo.hpp>
#include <snail/etl/parser/records/kernel/process.hpp>
#include <snail/etl/parser/records/kernel/stackwalk.hpp>
#include <snail/etl/parser/records/kernel/thread.hpp>
#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>
#include <snail/etl/parser/records/visual_studio/diagnostics_hub.hpp>

#include <snail/common/unicode.hpp>

using namespace snail;
using namespace snail::analysis::detail;

namespace {

bool is_kernel_address(std::uint64_t address, std::uint32_t pointer_size)
{
    // See https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/user-space-and-system-space
    return pointer_size == 4 ?
               address >= 0x80000000 :        // 32bit
               address >= 0x0000800000000000; // 64bit
}

} // namespace

etl_file_process_context::etl_file_process_context()
{
    register_event<etl::parser::system_config_v2_physical_disk_event_view>();
    register_event<etl::parser::system_config_v2_logical_disk_event_view>();
    register_event<etl::parser::process_v4_type_group1_event_view>();
    register_event<etl::parser::thread_v3_type_group1_event_view>();
    register_event<etl::parser::image_v2_load_event_view>();
    register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>();
    register_event<etl::parser::stackwalk_v2_stack_event_view>();
    register_event<etl::parser::vs_diagnostics_hub_target_profiling_started_event_view>();
}

etl_file_process_context::~etl_file_process_context()
{}

etl::dispatching_event_observer& etl_file_process_context::observer()
{
    return observer_;
}

void etl_file_process_context::finish()
{
    static constexpr std::string_view nt_drive_name_prefix = "\\Device\\HarddiskVolume";

    for(auto& [process_id, process_module_map] : modules_per_process)
    {
        for(auto& module : process_module_map.all_modules())
        {
            if(!module.file_name.starts_with(nt_drive_name_prefix)) continue;

            const auto    path_start        = module.file_name.find_first_of('\\', nt_drive_name_prefix.size());
            const auto    partition_num_str = std::string_view(module.file_name).substr(nt_drive_name_prefix.size(), path_start - nt_drive_name_prefix.size());
            std::uint32_t partition_num;
            auto          result = std::from_chars(partition_num_str.data(), partition_num_str.data() + partition_num_str.size(), partition_num);
            if(result.ec != std::errc{} || result.ptr != partition_num_str.data() + partition_num_str.size()) continue;

            auto partition_iter = nt_partition_to_dos_volume_mapping.find(partition_num);
            if(partition_iter == nt_partition_to_dos_volume_mapping.end()) continue;
            std::string new_file_name;
            const auto  remaining_file_name = std::string_view(module.file_name).substr(path_start);
            new_file_name.reserve(partition_iter->second.size() + remaining_file_name.size());

            std::copy(partition_iter->second.begin(), partition_iter->second.end(), std::back_inserter(new_file_name));
            std::copy(remaining_file_name.begin(), remaining_file_name.end(), std::back_inserter(new_file_name));

            module.file_name = new_file_name;
        }
    }
}

const std::unordered_map<etl_file_process_context::process_id_t, etl_file_process_context::profiler_process_info>& etl_file_process_context::profiler_processes() const
{
    return profiler_processes_;
}

template<typename T>
void etl_file_process_context::register_event()
{
    observer_.register_event<T>(
        [this](const etl::etl_file::header_data& file_header, const etl::common_trace_header& header, const T& event)
        {
            this->handle_event(file_header, header, event);
        });
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_v2_physical_disk_event_view& event)
{
    const auto disk_number     = event.disk_number();
    const auto partition_count = event.partition_count();

    number_of_partitions_per_disk[disk_number] = partition_count;
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_v2_logical_disk_event_view& event)
{
    const auto disk_number = event.disk_number();
    assert(event.drive_type() == etl::parser::drive_type::partition);

    auto global_partition_number = event.partition_number();
    for(const auto [disk, partition_count] : number_of_partitions_per_disk)
    {
        if(disk >= disk_number) break;
        global_partition_number += partition_count;
    }

    nt_partition_to_dos_volume_mapping[global_partition_number] = common::utf16_to_utf8<char>(event.drive_letter());
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                       header,
                                            const etl::parser::process_v4_type_group1_event_view& event)
{
    if(header.type != 1 && header.type != 3) return; // We do only handle start events

    processes.insert(event.process_id(), header.timestamp, process_data{.image_filename = std::string(event.image_filename()), .command_line = std::u16string(event.command_line())});
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                      header,
                                            const etl::parser::thread_v3_type_group1_event_view& event)
{
    if(header.type != 1 && header.type != 3) return; // We do only handle start events

    threads.insert(event.thread_id(), header.timestamp, thread_data{.process_id = event.process_id()});
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&              header,
                                            const etl::parser::image_v2_load_event_view& event)
{
    if(header.type != 10 && header.type != 3) return; // We do only handle load events

    auto& modules = modules_per_process[event.process_id()];

    modules.insert(module_info{
                       .base        = event.image_base(),
                       .size        = event.image_size(),
                       .file_name   = common::utf16_to_utf8<char>(event.file_name()),
                       .page_offset = 0},
                   header.timestamp);
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                            header,
                                            const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
{
    const auto* const thread = threads.find_at(event.thread_id(), header.timestamp);
    if(!thread) return;

    const auto process_id = thread->payload.process_id;

    auto& process_samples = samples_per_process[process_id];

    process_samples.push_back(sample_info{
        .thread_id           = thread->id,
        .timestamp           = header.timestamp,
        .instruction_pointer = event.instruction_pointer()});
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& file_header,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::stackwalk_v2_stack_event_view& event)
{
    const auto process_id = event.process_id();
    auto       iter       = samples_per_process.find(process_id);
    if(iter == samples_per_process.end()) return;

    const auto thread_id        = event.thread_id();
    const auto sample_timestamp = event.event_timestamp();

    auto& process_samples = iter->second;

    for(auto& sample : std::views::reverse(process_samples))
    {
        if(sample.timestamp < sample_timestamp) break;
        if(sample.timestamp > sample_timestamp) continue;

        assert(sample.timestamp == sample_timestamp);

        if(sample.thread_id != thread_id) continue;

        const auto starts_in_kernel   = is_kernel_address(event.stack().back(), file_header.pointer_size);
        auto&      sample_stack_index = starts_in_kernel ? sample.kernel_mode_stack : sample.user_mode_stack;

        assert(sample_stack_index == std::nullopt);
        sample_stack_index = stacks.insert(event.stack());
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                                            header,
                                            const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event)
{
    const auto process_id           = event.process_id();
    profiler_processes_[process_id] = profiler_process_info{
        .process_id      = process_id,
        .start_timestamp = header.timestamp};
}

const etl_file_process_context::process_info* etl_file_process_context::try_get_process_at(process_id_t process_id, timestamp_t timestamp) const
{
    return processes.find_at(process_id, timestamp);
}

std::pair<const module_info*, common::timestamp_t> etl_file_process_context::try_get_module_at(process_id_t process_id, instruction_pointer_t address, timestamp_t timestamp) const
{
    const auto iter = modules_per_process.find(process_id);
    if(iter == modules_per_process.end()) return {nullptr, 0};

    const auto& modules = iter->second;

    return modules.find(address, timestamp);
}

const std::vector<etl_file_process_context::sample_info>& etl_file_process_context::process_samples(process_id_t process_id) const
{
    auto iter = samples_per_process.find(process_id);
    if(iter != samples_per_process.end()) return iter->second;

    static const std::vector<etl_file_process_context::sample_info> empty;
    return empty;
}

const std::vector<etl_file_process_context::instruction_pointer_t>& etl_file_process_context::stack(std::size_t stack_index) const
{
    return stacks.get(stack_index);
}
