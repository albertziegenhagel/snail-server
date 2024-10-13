
#include <snail/analysis/detail/etl_file_process_context.hpp>

#include <charconv>
#include <ranges>

#include <utf8/cpp17.h>

#include <snail/common/cast.hpp>

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

using namespace snail;
using namespace snail::analysis;
using namespace snail::analysis::detail;

namespace {

bool is_kernel_address(std::uint64_t address, std::uint32_t pointer_size)
{
    // See https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/user-space-and-system-space
    return pointer_size == 4 ?
               address >= 0x8000'0000 :          // 32bit
               address >= 0x0000'8000'0000'0000; // 64bit
}

// The default `SampledProfile` events are always "Timer" PMC events, with the PMC source 0.
inline constexpr etl_file_process_context::sample_source_id_t default_timer_pmc_source = 0;

} // namespace

etl_file_process_context::etl_file_process_context()
{
    register_event<etl::parser::system_config_v3_cpu_event_view>();
    register_event<etl::parser::system_config_v2_physical_disk_event_view>();
    register_event<etl::parser::system_config_v2_logical_disk_event_view>();
    register_event<etl::parser::system_config_v5_pnp_event_view>();
    register_event<etl::parser::system_config_ex_v0_build_info_event_view>();
    register_event<etl::parser::system_config_ex_v0_system_paths_event_view>();
    register_event<etl::parser::system_config_ex_v0_volume_mapping_event_view>();
    register_event<etl::parser::process_v4_type_group1_event_view>();
    register_event<etl::parser::thread_v3_type_group1_event_view>();
    // register_event<etl::parser::thread_v4_context_switch_event_view>();
    observer_.register_event<etl::parser::thread_v4_context_switch_event_view>(
        [this](const etl::etl_file::header_data& file_header, const etl::any_group_trace_header& header, const etl::parser::thread_v4_context_switch_event_view& event)
        {
            this->handle_event(file_header, header, event);
        });
    register_event<etl::parser::image_v3_load_event_view>();
    register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>();
    register_event<etl::parser::perfinfo_v2_pmc_counter_profile_event_view>();
    register_event<etl::parser::perfinfo_v3_sampled_profile_interval_event_view>();
    register_event<etl::parser::perfinfo_v2_pmc_counter_config_event_view>();
    register_event<etl::parser::stackwalk_v2_stack_event_view>();
    register_event<etl::parser::stackwalk_v2_key_event_view>();
    register_event<etl::parser::stackwalk_v2_type_group1_event_view>();
    register_event<etl::parser::image_id_v2_dbg_id_pdb_info_event_view>();
    register_event<etl::parser::vs_diagnostics_hub_target_profiling_started_event_view>();
    register_event<etl::parser::snail_profiler_profile_target_event_view>();
}

etl_file_process_context::~etl_file_process_context() = default;

etl::dispatching_event_observer& etl_file_process_context::observer()
{
    return observer_;
}

void etl_file_process_context::finish()
{
    static constexpr std::string_view nt_drive_name_prefix       = "\\Device\\HarddiskVolume";
    static constexpr std::string_view nt_system_root_name_prefix = "\\SystemRoot\\";

    static constexpr std::string_view default_system_root = "C:\\WINDOWS\\";

    // Assign unique IDs to processes and threads.
    unique_process_id next_process_id{.key = 0x1'0000'0000};
    for(auto& [id, entries] : processes.all_entries())
    {
        for(auto& entry : entries)
        {
            entry.payload.unique_id = next_process_id;
            ++next_process_id.key;

            unique_process_id_to_key_[*entry.payload.unique_id] = process_key{id, entry.timestamp};

            threads_per_process_[*entry.payload.unique_id]; // initialize the map
        }

        modules_per_process_id_[id]; // initialize the map
    }
    unique_thread_id next_thread_id{.key = 0x2'0000'0000};
    for(auto& [id, entries] : threads.all_entries())
    {
        for(auto& entry : entries)
        {
            entry.payload.unique_id = next_thread_id;
            ++next_thread_id.key;

            unique_thread_id_to_key_[*entry.payload.unique_id] = thread_key{id, entry.timestamp};
        }
    }

    // Build threads per proccess map
    for(const auto& [process_id, process_threads] : threads_per_process_id_)
    {
        for(const auto& thread_key : process_threads)
        {
            const auto* const process = processes.find_at(process_id, thread_key.time);
            if(process == nullptr) continue;

            const auto* const thread = threads.find_at(thread_key.id, thread_key.time);
            if(thread == nullptr) continue;

            threads_per_process_[*process->payload.unique_id].insert(*thread->payload.unique_id);
        }
    }

    // Map NT to DOS paths
    for(auto& [process_id, process_module_map] : modules_per_process_id_)
    {
        for(auto& module : process_module_map.all_modules())
        {
            // First try NT to DOS maps from XPerf events
            // (we assume that all NT prefixes start with a backslash. Hopefully that is not to restrictive.)
            if(module.payload.filename.starts_with("\\"))
            {
                bool found_in_map = false;
                for(const auto& [nt_prefix, dos_prefix] : nt_to_dos_path_map_)
                {
                    if(!module.payload.filename.starts_with(nt_prefix)) continue;
                    module.payload.filename.replace(0, nt_prefix.size(), dos_prefix);
                    found_in_map = true;
                    break;
                }
                if(found_in_map) continue;
            }

            // Now try drive information that we extracted from system config events
            if(module.payload.filename.starts_with(nt_drive_name_prefix))
            {
                // First extract the partition number
                const auto    path_start        = module.payload.filename.find_first_of('\\', nt_drive_name_prefix.size());
                const auto    partition_num_str = std::string_view(module.payload.filename).substr(nt_drive_name_prefix.size(), path_start - nt_drive_name_prefix.size());
                std::uint32_t partition_num;
                auto          result = std::from_chars(partition_num_str.data(), partition_num_str.data() + partition_num_str.size(), partition_num);

                // The following check should never be true but we just want to be fault tolerant
                if(result.ec != std::errc{} || result.ptr != partition_num_str.data() + partition_num_str.size()) continue;

                // Look up the partition to DOS volume. If we don't have one, there is nothing we can do...
                auto partition_iter = nt_partition_to_dos_volume_mapping.find(partition_num);
                if(partition_iter == nt_partition_to_dos_volume_mapping.end()) continue;

                module.payload.filename.replace(0, path_start, partition_iter->second);
            }
            else if(module.payload.filename.starts_with(nt_system_root_name_prefix))
            {
                // Try to use the system root from an XPerf event if available. Otherwise fall back to the default.
                const auto system_root = system_root_ ? std::string_view(*system_root_) : default_system_root;
                module.payload.filename.replace(0, nt_system_root_name_prefix.size(), system_root);
            }
        }
    }

    // Just in case that we are missing the sample interval events, try to generate some names
    // for all event sources that are present.
    if(!sample_source_names_.contains(default_timer_pmc_source))
    {
        for(const auto& [tid, samples] : samples_per_thread_id_)
        {
            assert(samples != nullptr);
            if(samples->empty()) continue;
            sample_source_names_[default_timer_pmc_source] = utf8::utf8to16(std::format("Unknown ({})", default_timer_pmc_source));
            break;
        }
    }
    for(const auto& [tid, sample_storages] : pmc_samples_per_thread_id_)
    {
        for(const auto& storage : sample_storages)
        {
            if(sample_source_names_.contains(storage.source)) continue;
            assert(storage.samples != nullptr);
            if(storage.samples->empty()) continue;
            sample_source_names_[storage.source] = utf8::utf8to16(std::format("Unknown ({})", storage.source));
            break;
        }
    }

    // Accumulate all remaining context switch data to their threads
    for(const auto& [thread_os_id, context_switch_data] : last_context_switch_data_per_thread_id_)
    {
        auto* const start_thread = threads.find_at(thread_os_id, context_switch_data.first_time);
        auto* const end_thread   = threads.find_at(thread_os_id, context_switch_data.last_time);
        if(start_thread == nullptr && end_thread == nullptr) continue; // we are still missing this thread? just ignore the data...

        if(start_thread != end_thread)
        {
            // The collected information seems to span over multiple threads? Maybe we missed some
            // start/end thread events in-between, but how could we have multiple threads in the
            // thread history then? Not sure whether this can ever happen, so just ignore for now...
        }
        else
        {
            assert(start_thread != nullptr);

            start_thread->payload.context_switches += context_switch_data.exit_count;

            if(context_switch_data.pmc_counters_info.size() > start_thread->payload.pmc_counts.size())
            {
                start_thread->payload.pmc_counts.resize(context_switch_data.pmc_counters_info.size(), 0);
            }
            for(std::size_t counter_index = 0; counter_index < context_switch_data.pmc_counters_info.size(); ++counter_index)
            {
                start_thread->payload.pmc_counts[counter_index] += context_switch_data.pmc_counters_info[counter_index].total_count;
            }
        }
    }

    // In case we did not have any events that explicitly specify processes that were the profiling
    // targets, just take all processes that have samples.
    if(profiler_processes_.empty())
    {
        for(auto& [id, entries] : processes.all_entries())
        {
            for(const auto& process_entry : entries)
            {
                bool has_samples = false;
                for(const auto& unique_thread_id : get_process_threads(*process_entry.payload.unique_id))
                {
                    const auto        thread_key = unique_thread_id_to_key_.at(unique_thread_id);
                    const auto* const thread     = threads.find_at(thread_key.id, thread_key.time);
                    if(thread == nullptr) continue;

                    {
                        auto iter = samples_per_thread_id_.find(thread->id);
                        if(iter != samples_per_thread_id_.end() && !iter->second->empty())
                        {
                            has_samples = true;
                            break;
                        }
                    }
                    {
                        auto iter = pmc_samples_per_thread_id_.find(thread->id);
                        if(iter != pmc_samples_per_thread_id_.end() && !iter->second.empty())
                        {
                            has_samples = true;
                            break;
                        }
                    }
                }

                if(!has_samples) continue;

                profiler_processes_[process_key{process_entry.id, process_entry.timestamp}] = profiler_process_info{
                    .process_id      = process_entry.id,
                    .start_timestamp = process_entry.timestamp};
            }
        }
    }
}

etl_file_process_context::process_key etl_file_process_context::id_to_key(unique_process_id id) const
{
    return unique_process_id_to_key_.at(id);
}

etl_file_process_context::thread_key etl_file_process_context::id_to_key(unique_thread_id id) const
{
    return unique_thread_id_to_key_.at(id);
}

const std::unordered_map<etl_file_process_context::process_key, etl_file_process_context::profiler_process_info>& etl_file_process_context::profiler_processes() const
{
    return profiler_processes_;
}

const etl_file_process_context::process_history& etl_file_process_context::get_processes() const
{
    return processes;
}

const etl_file_process_context::thread_history& etl_file_process_context::get_threads() const
{
    return threads;
}

const std::set<unique_thread_id>& etl_file_process_context::get_process_threads(unique_process_id process_id) const
{
    return threads_per_process_.at(process_id);
}

const module_map<etl_file_process_context::module_data, etl_file_process_context::timestamp_t>& etl_file_process_context::get_modules(os_pid_t process_id) const
{
    return modules_per_process_id_.at(process_id);
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
                                            const etl::parser::system_config_v3_cpu_event_view& event)
{
    computer_name_          = event.computer_name();
    processor_architecture_ = event.processor_architecture();
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

    nt_partition_to_dos_volume_mapping[global_partition_number] = utf8::utf16to8(event.drive_letter());
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_v5_pnp_event_view& event)
{
    if(processor_name_) return;

    // TODO: add other processors?
    if(event.device_description() == u"Intel Processor")
    {
        processor_name_ = event.friendly_name();
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_ex_v0_build_info_event_view& event)
{
    if(os_name_) return;

    os_name_ = event.product_name();
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_ex_v0_system_paths_event_view& event)
{
    if(system_root_) return;

    system_root_ = utf8::utf16to8(event.system_windows_directory());
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::system_config_ex_v0_volume_mapping_event_view& event)
{
    nt_to_dos_path_map_[utf8::utf16to8(event.nt_path())] = utf8::utf16to8(event.dos_path());
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                       header,
                                            const etl::parser::process_v4_type_group1_event_view& event)
{
    if(header.type == 1 || header.type == 3) // load || dc_start
    {
        processes.insert(event.process_id(), header.timestamp,
                         process_data{
                             .image_filename = utf8::replace_invalid(event.image_filename()),
                             .command_line   = std::u16string(event.command_line()),
                             .end_time       = {},
                             .unique_id      = {},
                         });
    }
    else if(header.type == 2 || header.type == 4) // unload || dc_end
    {
        auto* const process = processes.find_at(event.process_id(), header.timestamp);
        if(process != nullptr)
        {
            process->payload.end_time = header.timestamp;
        }
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                      header,
                                            const etl::parser::thread_v3_type_group1_event_view& event)
{
    thread_info* thread = nullptr;
    if(header.type == 1 || header.type == 3) // start || dc_start
    {
        const auto name = event.thread_name();

        const auto is_new_thread = threads.insert(event.thread_id(), header.timestamp,
                                                  thread_data{
                                                      .process_id       = event.process_id(),
                                                      .end_time         = {},
                                                      .unique_id        = {},
                                                      .name             = name ? std::make_optional(std::u16string(*name)) : std::nullopt,
                                                      .context_switches = 0,
                                                      .pmc_counts       = {}});
        if(is_new_thread)
        {
            threads_per_process_id_[event.process_id()].emplace(event.thread_id(), header.timestamp);
        }

        thread = &threads.all_entries().at(event.thread_id()).back();
    }
    else if(header.type == 2 || header.type == 4) // end || dc_end
    {
        thread = threads.find_at(event.thread_id(), header.timestamp);
        if(thread != nullptr)
        {
            thread->payload.end_time = header.timestamp;
        }
    }

    // If we collected context switch data for a thread with this ID already,
    // we will attribute it to this thread.
    // In case that this is a start thread-event, we assume that it was only for
    // a few context switches that happened before the thread start event was emitted.
    // If this is a thread end event, this should be the correct thing to do.
    auto context_switch_iter = last_context_switch_data_per_thread_id_.find(event.thread_id());
    if(context_switch_iter != last_context_switch_data_per_thread_id_.end())
    {
        const auto& context_switch_data = context_switch_iter->second;

        if(thread != nullptr)
        {
            thread->payload.context_switches += context_switch_data.exit_count;

            if(context_switch_data.pmc_counters_info.size() > thread->payload.pmc_counts.size())
            {
                thread->payload.pmc_counts.resize(context_switch_data.pmc_counters_info.size(), 0);
            }
            for(std::size_t counter_index = 0; counter_index < context_switch_data.pmc_counters_info.size(); ++counter_index)
            {
                thread->payload.pmc_counts[counter_index] += context_switch_data.pmc_counters_info[counter_index].total_count;
            }
        }

        // Delete the old context switch data, so that we will start collecting data
        // for the next thread with the same id. If this is a start event, we do not
        // care that it's split and the remaining data should be caught at the next
        // end thread event. Not deleting it in case of a start event, would make us
        // accumulate the current count two times.
        last_context_switch_data_per_thread_id_.erase(context_switch_iter);
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::any_group_trace_header&                      header_variant,
                                            const etl::parser::thread_v4_context_switch_event_view& event)
{
    // Context switch events can occur before the thread start events.
    // Hence, we cannot rely on the thread being present in the `threads` history.
    // Instead, we will accumulate context switch data in temporary storage and then
    // assign it to the threads on the threads start/end events and flush any potentially
    // remaining data in the end.

    const auto header = etl::make_common_trace_header(header_variant);

    const auto old_thread_id = event.old_thread_id();
    const auto new_thread_id = event.new_thread_id();

    auto& old_thread_data = [this, old_thread_id, &header]() -> auto&
    {
        auto iter = last_context_switch_data_per_thread_id_.find(old_thread_id);
        if(iter == last_context_switch_data_per_thread_id_.end())
        {
            auto& data      = last_context_switch_data_per_thread_id_[old_thread_id];
            data.first_time = header.timestamp;
            return data;
        }
        return iter->second;
    }();

    auto& new_thread_data = [this, new_thread_id, &header]() -> auto&
    {
        auto iter = last_context_switch_data_per_thread_id_.find(new_thread_id);
        if(iter == last_context_switch_data_per_thread_id_.end())
        {
            auto& data      = last_context_switch_data_per_thread_id_[new_thread_id];
            data.first_time = header.timestamp;
            return data;
        }
        return iter->second;
    }();

    // If we really receive events in the correct order, the `max` is not necessary here,
    // but we still leave it in to be a little bit more defensive.
    old_thread_data.last_time = std::max(old_thread_data.last_time, header.timestamp);
    new_thread_data.last_time = std::max(new_thread_data.last_time, header.timestamp);

    ++old_thread_data.exit_count;
    ++new_thread_data.enter_count;

    const auto* perfinfo_header = std::get_if<etl::parser::perfinfo_trace_header_view>(&header_variant);
    if(perfinfo_header)
    {
        if(old_thread_data.pmc_counters_info.size() < perfinfo_header->ext_pmc_count())
        {
            old_thread_data.pmc_counters_info.resize(perfinfo_header->ext_pmc_count());
        }
        if(new_thread_data.pmc_counters_info.size() < perfinfo_header->ext_pmc_count())
        {
            new_thread_data.pmc_counters_info.resize(perfinfo_header->ext_pmc_count());
        }

        for(std::uint8_t counter_index = 0; counter_index < perfinfo_header->ext_pmc_count(); ++counter_index)
        {
            const auto pmc_counter_value = perfinfo_header->ext_pmc(counter_index);

            auto& old_thread_counter_info = old_thread_data.pmc_counters_info[counter_index];

            // accumulate counter difference to old thread but skip thread '0'.
            // Thread '0' is not a real thread (but the scheduler?) and we are never
            // interested in this one. Since it is a "thread" that appears to run on multiple
            // cores at the same time, timer differences would be invalid anyways
            // (or we would have to keep the track of the "enter" counter PER CORE,
            // which should not be necessary for any other thread, since they always
            // run on a single core between context switches)...
            if(old_thread_id != 0)
            {
                if(old_thread_counter_info.prev_enter_count != std::nullopt)
                {
                    if(*old_thread_counter_info.prev_enter_count > pmc_counter_value)
                    {
                        // counter overflow while the thread was running?
                        assert(false);
                        // constexpr auto max_counter_value = std::numeric_limits<std::int32_t>::max(); // Is this even remotely correct????
                        // old_thread_counter_info.total_count += (max_counter_value - *old_thread_counter_info.prev_enter_count);
                        old_thread_counter_info.total_count += pmc_counter_value;
                    }
                    else
                    {
                        old_thread_counter_info.total_count += (pmc_counter_value - *old_thread_counter_info.prev_enter_count);
                    }
                    old_thread_counter_info.prev_enter_count = std::nullopt;
                }
            }

            // and store counter offset to new thread
            auto& new_thread_counter_info = new_thread_data.pmc_counters_info[counter_index];

            new_thread_counter_info.prev_enter_count = pmc_counter_value;
        }
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/, const etl::common_trace_header& header, const etl::parser::image_v3_load_event_view& event)
{
    if(header.type != 10 && header.type != 3) return; // We do only handle load events

    auto& modules   = modules_per_process_id_[event.process_id()];
    auto& pdb_infos = modules_pdb_info_per_process_id_[event.process_id()];

    const auto image_base = event.image_base();

    std::optional<detail::pdb_info> pdb_info;
    if(!pdb_infos.empty())
    {
        for(auto iter = pdb_infos.rbegin(); iter != pdb_infos.rend(); ++iter)
        {
            if(iter->event_timestamp != header.timestamp ||
               iter->image_base != image_base) continue;

            pdb_info = std::move(iter->info);

            pdb_infos.erase(std::next(iter).base());
            break;
        }
    }

    modules.insert(module_info<module_data>{
                       .base    = image_base,
                       .size    = event.image_size(),
                       .payload = {
                                   .filename = utf8::utf16to8(event.file_name()),
                                   .checksum = event.image_checksum(),
                                   .pdb_info = std::move(pdb_info)}
    },
                   header.timestamp);
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                            header,
                                            const etl::parser::image_id_v2_dbg_id_pdb_info_event_view& event)
{
    auto& pdb_infos = modules_pdb_info_per_process_id_[event.process_id()];

    pdb_infos.push_back(pdb_info_storage{
        .event_timestamp = header.timestamp,
        .image_base      = event.image_base(),
        .info            = {
                            .pdb_name = std::string(event.pdb_file_name()),
                            .guid     = event.guid().instantiate(),
                            .age      = event.age()}
    });
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                            header,
                                            const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
{
    const auto thread_id      = event.thread_id();
    auto&      thread_samples = samples_per_thread_id_[thread_id];
    if(thread_samples == nullptr) thread_samples = std::make_unique<std::vector<sample_info>>();

    thread_samples->push_back(sample_info{
        .thread_id           = thread_id,
        .timestamp           = header.timestamp,
        .instruction_pointer = event.instruction_pointer(),
        .user_mode_stack     = {},
        .user_timestamp      = {},
        .kernel_mode_stack   = {},
        .kernel_timestamp    = {},
    });
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                                header,
                                            const etl::parser::perfinfo_v2_pmc_counter_profile_event_view& event)
{
    const auto thread_id              = event.thread_id();
    auto&      thread_sample_storages = pmc_samples_per_thread_id_[thread_id];

    const auto profile_source = event.profile_source();
    auto       iter           = std::ranges::find_if(thread_sample_storages, [profile_source](const pmc_sample_storage& entry)
                                                     { return entry.source == profile_source; });

    auto& samples = [iter, &thread_sample_storages, profile_source]() -> auto&
    {
        if(iter == thread_sample_storages.end())
        {
            thread_sample_storages.emplace_back();
            thread_sample_storages.back().source  = profile_source;
            thread_sample_storages.back().samples = std::make_unique<std::vector<sample_info>>();
            return *thread_sample_storages.back().samples;
        }
        assert(iter->samples != nullptr);
        return *iter->samples;
    }();

    samples.push_back(sample_info{
        .thread_id           = thread_id,
        .timestamp           = header.timestamp,
        .instruction_pointer = event.instruction_pointer(),
        .user_mode_stack     = {},
        .user_timestamp      = {},
        .kernel_mode_stack   = {},
        .kernel_timestamp    = {},
    });
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                                     header,
                                            const etl::parser::perfinfo_v3_sampled_profile_interval_event_view& event)
{
    if(header.type != 73) return; // only handle collection start events.

    // If we receive multiple start events for the same source, we simply overwrite the last stored names.
    // This assumes that the names do not change per source ID, which should hopefully be correct?
    sample_source_names_[common::narrow_cast<sample_source_id_t>(event.source())] = event.source_name();
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header& /*header*/,
                                            const etl::parser::perfinfo_v2_pmc_counter_config_event_view& event)
{
    // FIXME: If we receive multiple counter config events, the last one will simply overwrite all previous ones.
    //        Even though this is not correct, I think it is very unlikely (or even impossible?) that the
    //        tracked PMC events change while a single trace is being collected.
    //        How could we handle this correctly? Maybe we can assume that if a counter config event happens, we can apply
    //        the names for all events that have been collected so far and then apply the names of the next
    //        counter config event only to the collected data since the last counter config event?
    pmc_names_.resize(event.counter_count());
    for(std::uint32_t counter_index = 0; counter_index < event.counter_count(); ++counter_index)
    {
        pmc_names_[counter_index] = event.counter_name(counter_index);
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data&                 file_header,
                                            [[maybe_unused]] const etl::common_trace_header&  header,
                                            const etl::parser::stackwalk_v2_stack_event_view& event)
{
    const auto thread_id = event.thread_id();

    const auto sample_timestamp = event.event_timestamp();

    // We expect the samples to arrive before their stacks.
    assert(sample_timestamp <= header.timestamp);

    const auto attach_stack_to_samples = [this, &file_header, &header, &event, sample_timestamp](std::vector<sample_info>& samples) -> bool
    {
        // Try to find the sample for this stack by walking the last samples
        // backwards. We expect that there shouldn't be to many events
        // between the sample and its corresponding stacks.
        for(auto& sample : std::views::reverse(samples))
        {
            if(sample.timestamp < sample_timestamp) break;
            if(sample.timestamp > sample_timestamp) continue;

            assert(sample.timestamp == sample_timestamp);

            const auto starts_in_kernel       = is_kernel_address(event.stack().back(), file_header.pointer_size);
            auto&      sample_stack_index     = starts_in_kernel ? sample.kernel_mode_stack : sample.user_mode_stack;
            auto&      sample_stack_timestamp = starts_in_kernel ? sample.kernel_timestamp : sample.user_timestamp;

            // Usually, we should have one user mode stack and optionally one kernel mode stack.
            // But it seems that we can sometimes have multiple kernel mode stacks for a single sample.
            // In this case we just replace the first kernel mode stack. Maybe the right thing to do would
            // be to concatenate the stacks, but the kernel mode stacks are kind of useless anyways?! So,
            // for know, we just replace the old stack.
            assert(sample_stack_index == std::nullopt || starts_in_kernel);

            sample_stack_index     = stacks.insert(event.stack());
            sample_stack_timestamp = header.timestamp;

            return true;
        }
        return false;
    };

    // First try to attach to a matching regular sample
    auto regular_samples_iter = samples_per_thread_id_.find(thread_id);
    if(regular_samples_iter != samples_per_thread_id_.end())
    {
        assert(regular_samples_iter->second != nullptr);
        const auto success = attach_stack_to_samples(*regular_samples_iter->second);
        if(success) sources_with_stacks_.insert(default_timer_pmc_source);
    }

    // Then, additionally try to attach to all matching PMC samples
    auto pmc_samples_iter = pmc_samples_per_thread_id_.find(thread_id);
    if(pmc_samples_iter == pmc_samples_per_thread_id_.end()) return;

    for(auto& [pmc_source, thread_samples] : pmc_samples_iter->second)
    {
        assert(thread_samples != nullptr);
        const auto success = attach_stack_to_samples(*thread_samples);
        if(success) sources_with_stacks_.insert(pmc_source);
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            [[maybe_unused]] const etl::common_trace_header& header,
                                            const etl::parser::stackwalk_v2_key_event_view&  event)
{
    const auto thread_id = event.thread_id();

    const auto sample_timestamp = event.event_timestamp();

    // We expect the samples to arrive before their stacks.
    assert(sample_timestamp <= header.timestamp);

    const auto is_kernel_stack = header.type == 37;

    const auto remember_samples_for_stack = [this, &header, &event, sample_timestamp, is_kernel_stack](std::vector<sample_info>& samples) -> bool
    {
        // Try to find the sample for this stack reference by walking the last samples
        // backwards. We expect that there shouldn't be to many events
        // between the sample and its corresponding stacks.
        for(auto sample_iter = samples.rbegin(); sample_iter != samples.rend(); ++sample_iter)
        {
            auto& sample = *sample_iter;
            if(sample.timestamp < sample_timestamp) break;
            if(sample.timestamp > sample_timestamp) continue;

            assert(sample.timestamp == sample_timestamp);

            auto& cached_samples = cached_samples_per_stack_key_[event.stack_key()];

            // remember the sample, so that we can fill in the stack later
            cached_samples.push_back(sample_stack_ref{
                .samples               = &samples,
                .sample_index          = samples.size() - (sample_iter - samples.rbegin()) - 1,
                .is_kernel_stack       = is_kernel_stack,
                .stack_event_timestamp = header.timestamp});

            return true;
        }
        return false;
    };

    // First try to remember matching regular samples
    auto regular_samples_iter = samples_per_thread_id_.find(thread_id);
    if(regular_samples_iter != samples_per_thread_id_.end())
    {
        assert(regular_samples_iter->second != nullptr);
        const auto success = remember_samples_for_stack(*regular_samples_iter->second);
        if(success) sources_with_stacks_.insert(default_timer_pmc_source);
    }

    // Then, additionally try to remember to all matching PMC samples
    auto pmc_samples_iter = pmc_samples_per_thread_id_.find(thread_id);
    if(pmc_samples_iter == pmc_samples_per_thread_id_.end()) return;

    for(auto& [pmc_source, thread_samples] : pmc_samples_iter->second)
    {
        assert(thread_samples != nullptr);
        const auto success = remember_samples_for_stack(*thread_samples);
        if(success) sources_with_stacks_.insert(pmc_source);
    }
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data&                       file_header,
                                            [[maybe_unused]] const etl::common_trace_header&        header,
                                            const etl::parser::stackwalk_v2_type_group1_event_view& event)
{
    if(header.type == 34) return; // Not sure what to do with a 'create' event. Are they ever emitted?

    // check whether we remembered any samples that require this stack
    auto iter = cached_samples_per_stack_key_.find(event.stack_key());
    if(iter == cached_samples_per_stack_key_.end()) return;

    // insert the new stack into our own cache
    const auto new_stack_index = stacks.insert(event.stack());

    [[maybe_unused]] const auto starts_in_kernel = is_kernel_address(event.stack().back(), file_header.pointer_size);

    // and apply it to all the samples that have been remembered for it
    for(const auto& sample_ref : iter->second)
    {
        auto& sample = (*sample_ref.samples)[sample_ref.sample_index];
        if(sample_ref.is_kernel_stack)
        {
            assert(starts_in_kernel);
            sample.kernel_mode_stack = new_stack_index;
            sample.kernel_timestamp  = sample_ref.stack_event_timestamp;
        }
        else
        {
            assert(!starts_in_kernel);
            assert(sample.user_mode_stack == std::nullopt);
            sample.user_mode_stack = new_stack_index;
            sample.user_timestamp  = sample_ref.stack_event_timestamp;
        }
    }

    // now, delete the remembered samples for this stack key (since the key could be reused for a new stack).
    cached_samples_per_stack_key_.erase(iter);
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                                            header,
                                            const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event)
{
    const auto process_id = event.process_id();

    profiler_processes_[process_key{process_id, header.timestamp}] = profiler_process_info{
        .process_id      = process_id,
        .start_timestamp = header.timestamp};
}

void etl_file_process_context::handle_event(const etl::etl_file::header_data& /*file_header*/,
                                            const etl::common_trace_header&                              header,
                                            const etl::parser::snail_profiler_profile_target_event_view& event)
{
    const auto process_id = event.process_id();

    profiler_processes_[process_key{process_id, header.timestamp}] = profiler_process_info{
        .process_id      = process_id,
        .start_timestamp = header.timestamp};
}

const std::unordered_map<etl_file_process_context::sample_source_id_t, std::u16string>& etl_file_process_context::sample_source_names() const
{
    return sample_source_names_;
}

bool etl_file_process_context::sample_source_has_stacks(sample_source_id_t pmc_source) const
{
    return sources_with_stacks_.contains(pmc_source);
}

std::span<const etl_file_process_context::sample_info> etl_file_process_context::thread_samples(os_tid_t                   thread_id,
                                                                                                timestamp_t                start_time,
                                                                                                std::optional<timestamp_t> end_time,
                                                                                                sample_source_id_t         pmc_source) const
{
    const auto samples = [this, thread_id, pmc_source]() -> std::span<const sample_info>
    {
        // First, try to find in the given source in the PMC samples. If we can't find the source
        // there but the source is actually the default timer source, fall back to using the
        // regular samples.
        auto iter = pmc_samples_per_thread_id_.find(thread_id);
        if(iter == pmc_samples_per_thread_id_.end())
        {
            if(pmc_source == default_timer_pmc_source)
            {
                auto iter2 = samples_per_thread_id_.find(thread_id);
                if(iter2 == samples_per_thread_id_.end()) return {};
                assert(iter2->second != nullptr);
                return std::span(*iter2->second);
            }
            return {};
        }

        const auto& thread_sample_storages = iter->second;

        auto iter2 = std::ranges::find_if(thread_sample_storages, [pmc_source](const pmc_sample_storage& entry)
                                          { return entry.source == pmc_source; });
        if(iter2 == thread_sample_storages.end())
        {
            if(pmc_source == default_timer_pmc_source)
            {
                auto iter3 = samples_per_thread_id_.find(thread_id);
                if(iter3 == samples_per_thread_id_.end()) return {};
                assert(iter3->second != nullptr);
                return std::span(*iter3->second);
            }
            return {};
        }
        assert(iter2->samples != nullptr);
        return std::span(*iter2->samples);
    }();

    if(samples.empty()) return {};

    const auto range_first_iter = std::ranges::lower_bound(samples, start_time, std::less<>(), &sample_info::timestamp);
    if(range_first_iter == samples.end()) return {};

    const auto range_end_iter = end_time != std::nullopt ?
                                    std::ranges::upper_bound(samples, *end_time, std::less<>(), &sample_info::timestamp) :
                                    samples.end();

    if(range_first_iter > range_end_iter) return {};

    return samples.subspan(range_first_iter - samples.begin(), range_end_iter - range_first_iter);
}

const std::vector<etl_file_process_context::instruction_pointer_t>& etl_file_process_context::stack(std::size_t stack_index) const
{
    return stacks.get(stack_index);
}

std::optional<std::u16string_view> etl_file_process_context::computer_name() const
{
    if(!computer_name_) return std::nullopt;
    return *computer_name_;
}

std::optional<std::uint16_t> etl_file_process_context::processor_architecture() const
{
    return processor_architecture_;
}

std::optional<std::u16string_view> etl_file_process_context::processor_name() const
{
    if(!processor_name_) return std::nullopt;
    return *processor_name_;
}

std::optional<std::u16string_view> etl_file_process_context::os_name() const
{
    if(!os_name_) return std::nullopt;
    return *os_name_;
}

std::optional<std::u16string_view> etl_file_process_context::pmc_name(std::size_t counter_index) const
{
    if(counter_index >= pmc_names_.size()) return std::nullopt;
    return pmc_names_[counter_index];
}
