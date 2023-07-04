
#include <snail/etl/etl_file.hpp>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <optional>
#include <stdexcept>
#include <string_view>

#include <utf8/cpp17.h>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/etl/parser/records/kernel/config.hpp>
#include <snail/etl/parser/records/kernel/header.hpp>
#include <snail/etl/parser/records/kernel/image.hpp>
#include <snail/etl/parser/records/kernel/perfinfo.hpp>
#include <snail/etl/parser/records/kernel/process.hpp>
#include <snail/etl/parser/records/kernel/stackwalk.hpp>
#include <snail/etl/parser/records/kernel/thread.hpp>

#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>
#include <snail/etl/parser/records/kernel_trace_control/system_config_ex.hpp>

#include <snail/etl/parser/records/visual_studio/diagnostics_hub.hpp>

#include <snail/common/detail/dump.hpp>
#include <snail/common/types.hpp>

using namespace snail;

namespace {

std::string_view group_to_string(etl::parser::event_trace_group group)
{
    switch(group)
    {
    case etl::parser::event_trace_group::header: return "header";
    case etl::parser::event_trace_group::io: return "io";
    case etl::parser::event_trace_group::memory: return "memory";
    case etl::parser::event_trace_group::process: return "process";
    case etl::parser::event_trace_group::file: return "file";
    case etl::parser::event_trace_group::thread: return "thread";
    case etl::parser::event_trace_group::tcpip: return "tcpip";
    case etl::parser::event_trace_group::job: return "job";
    case etl::parser::event_trace_group::udpip: return "udpip";
    case etl::parser::event_trace_group::registry: return "registry";
    case etl::parser::event_trace_group::dbgprint: return "dbgprint";
    case etl::parser::event_trace_group::config: return "config";
    case etl::parser::event_trace_group::spare1: return "spare1";
    case etl::parser::event_trace_group::wnf: return "wnf";
    case etl::parser::event_trace_group::pool: return "pool";
    case etl::parser::event_trace_group::perfinfo: return "perfinfo";
    case etl::parser::event_trace_group::heap: return "heap";
    case etl::parser::event_trace_group::object: return "object";
    case etl::parser::event_trace_group::power: return "power";
    case etl::parser::event_trace_group::modbound: return "modbound";
    case etl::parser::event_trace_group::image: return "image";
    case etl::parser::event_trace_group::dpc: return "dpc";
    case etl::parser::event_trace_group::cc: return "cc";
    case etl::parser::event_trace_group::critsec: return "critsec";
    case etl::parser::event_trace_group::stackwalk: return "stackwalk";
    case etl::parser::event_trace_group::ums: return "ums";
    case etl::parser::event_trace_group::alpc: return "alpc";
    case etl::parser::event_trace_group::splitio: return "splitio";
    case etl::parser::event_trace_group::thread_pool: return "thread_pool";
    case etl::parser::event_trace_group::hypervisor: return "hypervisor";
    case etl::parser::event_trace_group::hypervisorx: return "hypervisorx";
    }
    return "UNKNOWN";
}

template<typename T>
    requires std::same_as<T, etl::parser::system_trace_header_view> ||
             std::same_as<T, etl::parser::compact_trace_header_view> ||
             std::same_as<T, etl::parser::perfinfo_trace_header_view>
auto make_key(const T& trace_header)
{
    return etl::detail::group_handler_key{
        trace_header.packet().group(),
        trace_header.packet().type(),
        trace_header.version()};
}

template<typename T>
    requires std::same_as<T, etl::parser::full_header_trace_header_view> ||
             std::same_as<T, etl::parser::instance_trace_header_view>
auto make_key(const T& trace_header)
{
    return etl::detail::guid_handler_key{
        trace_header.guid().instantiate(),
        trace_header.trace_class().type(),
        trace_header.trace_class().version()};
}

auto make_key(const etl::parser::event_header_trace_header_view& trace_header)
{
    return etl::detail::guid_handler_key{
        trace_header.provider_id().instantiate(),
        trace_header.event_descriptor().id(),
        trace_header.event_descriptor().version()};
}

std::string extract_application_name(std::string_view application_path)
{
    if(application_path.empty()) return "etl_file";
    return std::filesystem::path(application_path).filename().string();
}

void print_usage(std::string_view application_path)
{
    std::cout << std::format("Usage: {} <Options> <File>", extract_application_name(application_path)) << "\n"
              << "\n"
              << "File:\n"
              << "  Path to the *.etl file to be read.\n"
              << "Options:\n"
              << "  --help, -h       Show this help text.\n"
              << "  --unsupported    Print unsupported events summary.\n"
              << "  --dump           Dump event buffers.\n"
              << "  --header         Print header event.\n"
              << "  --config         Print kernel config events.\n"
              << "  --perfinfo       Print kernel perfinfo events (does not include samples).\n"
              << "  --config-ex      Print XPerf extended config events.\n"
              << "  --vs-diag        Print events from the VS Diagnostics Hub.\n"
              << "  --pid <pid>      Process ID of the process of interest. Pass 'any' to show\n"
              << "                   information for all processes.\n"
              << "  --only <event>   Print only events that match the given names.\n"
              << "  --except <event> Do not print events that match the given names.\n";
}

[[noreturn]] void print_usage_and_exit(std::string_view application_path, int exit_code)
{
    print_usage(application_path);
    std::quick_exit(exit_code);
}

[[noreturn]] void print_error_and_exit(std::string_view application_path, std::string_view error)
{
    std::cout << "Error:\n"
              << "  " << error << "\n\n";
    print_usage_and_exit(application_path, EXIT_FAILURE);
}

struct options
{
    std::filesystem::path file_path;

    bool show_unsupported_summary = false;

    bool dump = false;

    bool show_header    = false;
    bool show_config    = false;
    bool show_perfinfo  = false;
    bool show_config_ex = false;
    bool show_vs_diag   = false;

    std::optional<common::process_id_t> process_of_interest;
    bool                                all_processes = false;

    std::vector<std::string> only;
    std::vector<std::string> except;
};

options parse_command_line(int argc, char* argv[]) // NOLINT(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
{
    const auto application_path = argc > 0 ? std::string_view(argv[0]) : "";

    options                              result;
    bool                                 help = false;
    std::optional<std::filesystem::path> etl_file_path;
    for(int arg_i = 1; arg_i < argc; ++arg_i)
    {
        const auto current_arg = std::string_view(argv[arg_i]);
        if(current_arg == "--help" || current_arg == "-h")
        {
            help = true;
        }
        else if(current_arg == "--unsupported")
        {
            result.show_unsupported_summary = true;
        }
        else if(current_arg == "--dump")
        {
            result.dump = true;
        }
        else if(current_arg == "--header")
        {
            result.show_header = true;
        }
        else if(current_arg == "--config")
        {
            result.show_config = true;
        }
        else if(current_arg == "--perfinfo")
        {
            result.show_perfinfo = true;
        }
        else if(current_arg == "--config-ex")
        {
            result.show_config_ex = true;
        }
        else if(current_arg == "--vs-diag")
        {
            result.show_vs_diag = true;
        }
        else if(current_arg == "--pid")
        {
            if(argc <= arg_i + 1) print_error_and_exit(application_path, "Missing argument for --pid.");
            const auto next_arg = std::string_view(argv[++arg_i]);
            if(next_arg == "any")
            {
                result.all_processes = true;
            }
            else
            {
                result.process_of_interest = atoi(next_arg.data()); // We know that this is actually null-terminated
            }
        }
        else if(current_arg == "--only")
        {
            if(argc <= arg_i + 1) print_error_and_exit(application_path, "Missing argument for --only.");
            const auto next_arg = std::string_view(argv[++arg_i]);
            result.only.emplace_back(next_arg);
        }
        else if(current_arg == "--except")
        {
            if(argc <= arg_i + 1) print_error_and_exit(application_path, "Missing argument for --only.");
            const auto next_arg = std::string_view(argv[++arg_i]);
            result.except.emplace_back(next_arg);
        }
        else if(current_arg.starts_with("-"))
        {
            print_error_and_exit(application_path, std::format("Unknown command line argument: {}", current_arg));
        }
        else if(etl_file_path != std::nullopt)
        {
            print_error_and_exit(application_path, std::format("Multiple files not supported: first was '' second is '{}'", etl_file_path->string(), current_arg));
        }
        else
        {
            etl_file_path = current_arg;
        }
    }

    if(help)
    {
        print_usage_and_exit(application_path, EXIT_SUCCESS);
    }

    if(!etl_file_path)
    {
        print_error_and_exit(application_path, "Missing *.etl file.");
    }
    result.file_path = *etl_file_path;

    return result;
}

bool should_ignore(const options& opts, std::string_view event_name)
{
    if(!opts.only.empty())
    {
        bool pass = false;
        for(const auto& filter : opts.only)
        {
            if(event_name.starts_with(filter))
            {
                pass = true;
                break;
            }
        }
        if(!pass) return true;
    }
    if(!opts.except.empty())
    {
        bool pass = true;
        for(const auto& filter : opts.except)
        {
            if(event_name.starts_with(filter))
            {
                pass = false;
                break;
            }
        }
        if(!pass) return true;
    }
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    const auto options = parse_command_line(argc, argv);

    etl::etl_file file;

    try
    {
        std::cout << std::format("File: {}\n", options.file_path.string());
        file.open(options.file_path);
    }
    catch(const std::exception& e)
    {
        std::cerr << std::format("Failed to open ETL file: {}\n", e.what());
        return EXIT_FAILURE;
    }

    etl::dispatching_event_observer observer;

    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  unprocessed_guid_event_counts;
    std::unordered_map<etl::detail::group_handler_key, std::size_t> unprocessed_group_event_counts;
    std::size_t                                                     sample_count = 0;
    std::size_t                                                     stack_count  = 0;

    const std::unordered_map<etl::detail::guid_handler_key, std::string> known_guid_event_names = {
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 32, 2},         "XPerf:ImageIdV2-DbgIdNone"        },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 38, 2},         "XPerf:ImageIdV2-DbgIdPPdb"        },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 40, 1},         "XPerf:ImageIdV2-DbgIdDeterm"      },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 64, 0},         "XPerf:ImageIdV2-DbgIdFileVersion" },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 34, 0}, "XPerf:SysConfigExV0-UnknownVolume"}
    };
    const std::unordered_map<etl::detail::group_handler_key, std::string> known_group_event_names = {
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 5, 2},   "Kernel:EventTraceV2-ExtensionTypeGroup-5:Extension"    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 32, 2},  "Kernel:EventTraceV2-ExtensionTypeGroup-32:EndExtension"},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 8, 2},   "Kernel:EventTraceV2-RDComplete"                        },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 64, 2},  "Kernel:EventTraceV2-DbgIdRSDS"                         },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 66, 2},  "Kernel:EventTraceV2-BuildInfo"                         },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 11, 2}, "Kernel:ProcessV2-Terminate"                            },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 39, 5}, "Kernel:ProcessV5-Defunct"                              },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 33, 2},   "Kernel:ImageV2-KernelImageBase"                        },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 34, 2},   "Kernel:ImageV2-HypercallPage"                          },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 10, 3},  "Kernel:SysConfigV3-Cpu"                                },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 13, 2},  "Kernel:SysConfigV2-Nic"                                },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 14, 2},  "Kernel:SysConfigV2-Video"                              },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 15, 3},  "Kernel:SysConfigV3-Services"                           },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 16, 2},  "Kernel:SysConfigV2-Power"                              },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 21, 3},  "Kernel:SysConfigV3-Irq"                                },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 25, 2},  "Kernel:SysConfigV2-Platform"                           },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 28, 2},  "Kernel:SysConfigV2-Dpi"                                },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 29, 2},  "Kernel:SysConfigV2-CodeIntegrity"                      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 30, 2},  "Kernel:SysConfigV2-TelemetryInfo"                      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 31, 2},  "Kernel:SysConfigV2-Defrag"                             },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 33, 2},  "Kernel:SysConfigV2-DeviceFamily"                       },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 34, 2},  "Kernel:SysConfigV2-FlightIds"                          },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 35, 2},  "Kernel:SysConfigV2-Processors"                         },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 36, 2},  "Kernel:SysConfigV2-Virtualization"                     },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 37, 2},  "Kernel:SysConfigV2-Boot"                               }
    };

    // Unknown events
    {
        observer.register_unknown_event(
            [&unprocessed_guid_event_counts]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                             const etl::any_guid_trace_header&                  header,
                                             [[maybe_unused]] const std::span<const std::byte>& event_data)
            {
                const auto header_key = std::visit([](const auto& header)
                                                   { return make_key(header); },
                                                   header);
                ++unprocessed_guid_event_counts[header_key];
            });
        observer.register_unknown_event(
            [&unprocessed_group_event_counts]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                              const etl::any_group_trace_header&                 header,
                                              [[maybe_unused]] const std::span<const std::byte>& event_data)
            {
                const auto header_key = std::visit([](const auto& header)
                                                   { return make_key(header); },
                                                   header);
                ++unprocessed_group_event_counts[header_key];
            });
    }

    // Kernel: header
    {
        observer.register_event<etl::parser::event_trace_v2_header_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                       [[maybe_unused]] const etl::common_trace_header&     header,
                       const etl::parser::event_trace_v2_header_event_view& event)
            {
                if(!options.show_header) return;
                const auto event_name = "Kernel:EventTraceV2-Header";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}:\n", header.timestamp, event_name);
                std::cout << std::format("    buffer_size:           {}\n", event.buffer_size());
                std::cout << std::format("    version:               {}\n", event.version());
                std::cout << std::format("    provider_version:      {}\n", event.provider_version());
                std::cout << std::format("    number_of_processors:  {}\n", event.number_of_processors());
                std::cout << std::format("    end_time:              {}\n", event.end_time());
                std::cout << std::format("    timer_resolution:      {}\n", event.timer_resolution());
                std::cout << std::format("    max_file_size:         {}\n", event.max_file_size());
                std::cout << std::format("    log_file_mode:         {}\n", event.log_file_mode());
                std::cout << std::format("    buffers_written:       {}\n", event.buffers_written());
                std::cout << std::format("    start_buffers:         {}\n", event.start_buffers());
                std::cout << std::format("    pointer_size:          {}\n", event.pointer_size());
                std::cout << std::format("    events_lost:           {}\n", event.events_lost());
                std::cout << std::format("    logger_name:           {}\n", event.logger_name());
                std::cout << std::format("    log_file_name:         {}\n", event.log_file_name());
                // std::cout << std::format("    time_zone_information: {}\n", event.time_zone_information());
                std::cout << std::format("    boot_time:             {}\n", event.boot_time());
                std::cout << std::format("    perf_freq:             {}\n", event.perf_freq());
                std::cout << std::format("    start_time:            {}\n", event.start_time());
                std::cout << std::format("    reserved_flags:        {}\n", event.reserved_flags());
                std::cout << std::format("    buffers_lost:          {}\n", event.buffers_lost());
                std::cout << std::format("    session_name:          '{}'\n", utf8::utf16to8(event.session_name()));
                std::cout << std::format("    file_name:             '{}'\n", utf8::utf16to8(event.file_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel: config
    {
        observer.register_event<etl::parser::system_config_v3_cpu_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                       [[maybe_unused]] const etl::common_trace_header&    header,
                       const etl::parser::system_config_v3_cpu_event_view& event)
            {
                if(!options.show_config) return;
                const auto event_name = "Kernel:SysConfigV3-CPU";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: computer_name '{}' architecture {}...\n", header.timestamp, event_name, utf8::utf16to8(event.computer_name()), event.processor_architecture());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::system_config_v2_physical_disk_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&            file_header,
                       [[maybe_unused]] const etl::common_trace_header&              header,
                       const etl::parser::system_config_v2_physical_disk_event_view& event)
            {
                if(!options.show_config) return;
                const auto event_name = "Kernel:SysConfigV2-PhyDisk";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: disk_number {} partition_count {} ...\n", header.timestamp, event_name, event.disk_number(), event.partition_count());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::system_config_v2_logical_disk_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&           file_header,
                       [[maybe_unused]] const etl::common_trace_header&             header,
                       const etl::parser::system_config_v2_logical_disk_event_view& event)
            {
                if(!options.show_config) return;
                const auto event_name = "Kernel:SysConfigV2-LogDisk";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: disk_number {} partition_number {} drive_letter {} ...\n", header.timestamp, event_name, event.disk_number(), event.partition_number(), utf8::utf16to8(event.drive_letter()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::system_config_v5_pnp_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                       [[maybe_unused]] const etl::common_trace_header&    header,
                       const etl::parser::system_config_v5_pnp_event_view& event)
            {
                if(!options.show_config) return;
                const auto event_name = "Kernel:SysConfigV5-PNP";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: device_description '{}' friendly_name '{}' ...\n", header.timestamp, event_name, utf8::utf16to8(event.device_description()), utf8::utf16to8(event.friendly_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel: perfinfo/stack
    {
        observer.register_event<etl::parser::stackwalk_v2_stack_event_view>(
            [&options, &stack_count]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                     [[maybe_unused]] const etl::common_trace_header&   header,
                                     const etl::parser::stackwalk_v2_stack_event_view&  event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                ++stack_count;
            });
        observer.register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>(
            [&options, &sample_count]([[maybe_unused]] const etl::etl_file::header_data&         file_header,
                                      [[maybe_unused]] const etl::common_trace_header&           header,
                                      const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
            {
                const auto thread_id  = event.thread_id();
                const auto process_id = thread_id; // TODO: map thread to process
                if(!options.all_processes && process_id != options.process_of_interest) return;

                ++sample_count;
            });
        observer.register_event<etl::parser::perfinfo_v3_sampled_profile_interval_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&                  file_header,
                       [[maybe_unused]] const etl::common_trace_header&                    header,
                       const etl::parser::perfinfo_v3_sampled_profile_interval_event_view& event)
            {
                if(!options.show_perfinfo) return;

                const auto event_name = std::format("Kernel:PerfInfoV3-SampledProfileInterval-{}", header.type);
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: source {} new-interval {} old-interval {} source-name '{}'\n", header.timestamp, event_name, event.source(), event.new_interval(), event.old_interval(), utf8::utf16to8(event.source_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel: process/thread
    {
        observer.register_event<etl::parser::process_v4_type_group1_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&    file_header,
                       const etl::common_trace_header&                       header,
                       const etl::parser::process_v4_type_group1_event_view& event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = std::format("Kernel:ProcessV4-Group1-{}", header.type);
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} filename '{}' cmd '{}' unique {} flags {} ...\n", header.timestamp, event_name, process_id, event.image_filename(), utf8::utf16to8(event.command_line()), event.unique_process_key(), event.flags());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::thread_v3_type_group1_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                       const etl::common_trace_header&                      header,
                       const etl::parser::thread_v3_type_group1_event_view& event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = std::format("Kernel:ThreadV3-Group1-{}", header.type);
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} ...\n", header.timestamp, event_name, process_id, event.thread_id());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::thread_v4_type_group1_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                       const etl::common_trace_header&                      header,
                       const etl::parser::thread_v4_type_group1_event_view& event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = std::format("Kernel:ThreadV4-Group1-{}", header.type);
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} name {}...\n", header.timestamp, event_name, process_id, event.thread_id(), utf8::utf16to8(event.thread_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::thread_v2_set_name_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                       const etl::common_trace_header&                    header,
                       const etl::parser::thread_v2_set_name_event_view&  event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = "Kernel:ThreadV2-SetName";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} name '{}'\n", header.timestamp, event_name, process_id, event.thread_id(), utf8::utf16to8(event.thread_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel: image
    {
        observer.register_event<etl::parser::image_v3_load_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                       const etl::common_trace_header&                    header,
                       const etl::parser::image_v3_load_event_view&       event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = std::format("Kernel:ImageV3-Load-{}", header.type);
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} name '{}' image-base {:#0x} checksum {} time-date-stamp {}\n", header.timestamp, event_name, process_id, utf8::utf16to8(event.file_name()), event.image_base(), event.image_checksum(), event.time_date_stamp());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel-trace-control (Xperf): image
    {
        observer.register_event<etl::parser::image_id_v2_info_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                       const etl::common_trace_header&                    header,
                       const etl::parser::image_id_v2_info_event_view&    event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = "XPerf:ImageIdV2-Info";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} name '{}' image_base {:#0x} time_stamp {}\n", header.timestamp, event_name, process_id, utf8::utf16to8(event.original_file_name()), event.image_base(), event.time_date_stamp());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::image_id_v2_dbg_id_pdb_info_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&         file_header,
                       const etl::common_trace_header&                            header,
                       const etl::parser::image_id_v2_dbg_id_pdb_info_event_view& event)
            {
                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                const auto event_name = "XPerf:ImageIdV2-DbgIdPdbInfo";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} image-base {} guid {} age {} pdb '{}'\n", header.timestamp, event_name, process_id, event.image_base(), event.guid().instantiate().to_string(), event.age(), event.pdb_file_name());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // Kernel-trace-control (Xperf): system-config-ex
    {
        observer.register_event<etl::parser::system_config_ex_v0_build_info_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&            file_header,
                       const etl::common_trace_header&                               header,
                       const etl::parser::system_config_ex_v0_build_info_event_view& event)
            {
                if(!options.show_config_ex) return;
                const auto event_name = "XPerf:SysConfigExV0-BuildInfo";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: install-date {} build-lab '{}' product-name '{}'\n", header.timestamp, event_name, event.install_date(), utf8::utf16to8(event.build_lab()), utf8::utf16to8(event.product_name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::system_config_ex_v0_system_paths_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&              file_header,
                       const etl::common_trace_header&                                 header,
                       const etl::parser::system_config_ex_v0_system_paths_event_view& event)
            {
                if(!options.show_config_ex) return;
                const auto event_name = "XPerf:SysConfigExV0-SysPaths";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: sys-dir '{}' sys-win-dir '{}'\n", header.timestamp, event_name, utf8::utf16to8(event.system_directory()), utf8::utf16to8(event.system_windows_directory()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::system_config_ex_v0_volume_mapping_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&                file_header,
                       const etl::common_trace_header&                                   header,
                       const etl::parser::system_config_ex_v0_volume_mapping_event_view& event)
            {
                if(!options.show_config_ex) return;
                const auto event_name = "XPerf:SysConfigExV0-VolumeMapping";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: nt-path '{}' dos-path '{}'\n", header.timestamp, event_name, utf8::utf16to8(event.nt_path()), utf8::utf16to8(event.dos_path()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    // VS-Diagnostics-Hub
    {
        observer.register_event<etl::parser::vs_diagnostics_hub_target_profiling_started_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&                         file_header,
                       const etl::common_trace_header&                                            header,
                       const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event)
            {
                if(!options.show_vs_diag) return;
                const auto event_name = "VsDiagHub:TargetProfilingStarted";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} timestamp {}\n", header.timestamp, event_name, event.process_id(), event.timestamp());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::vs_diagnostics_hub_target_profiling_stopped_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&                         file_header,
                       const etl::common_trace_header&                                            header,
                       const etl::parser::vs_diagnostics_hub_target_profiling_stopped_event_view& event)
            {
                if(!options.show_vs_diag) return;
                const auto event_name = "VsDiagHub:TargetProfilingStopped";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} timestamp {}\n", header.timestamp, event_name, event.process_id(), event.timestamp());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::vs_diagnostics_hub_machine_info_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&             file_header,
                       [[maybe_unused]] const etl::common_trace_header&               header,
                       const etl::parser::vs_diagnostics_hub_machine_info_event_view& event)
            {
                if(!options.show_vs_diag) return;
                const auto event_name = "VsDiagHub:MachineInfo";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: architecture {} os '{}' name '{}'\n", header.timestamp, event_name, (int)event.architecture(), utf8::utf16to8(event.os_description()), utf8::utf16to8(event.name()));

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
        observer.register_event<etl::parser::vs_diagnostics_hub_counter_info_event_view>(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&             file_header,
                       [[maybe_unused]] const etl::common_trace_header&               header,
                       const etl::parser::vs_diagnostics_hub_counter_info_event_view& event)
            {
                if(!options.show_vs_diag) return;
                const auto event_name = "VsDiagHub:CounterInfo";
                if(should_ignore(options, event_name)) return;

                std::cout << std::format("@{} {:30}: Count {} timestamp {} value {}\n", header.timestamp, event_name, (int)event.counter(), event.timestamp(), event.value());

                if(options.dump) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            });
    }

    std::cout << "\n";
    file.process(observer);

    std::cout << "\n";
    std::cout << std::format("Number of samples: {}\n", sample_count);
    std::cout << std::format("Number of stacks:  {}\n", stack_count);

    if(options.show_unsupported_summary)
    {
        std::cout << "\n";
        std::cout << "Unsupported events:\n";
        for(const auto& [key, count] : unprocessed_group_event_counts)
        {
            if(known_group_event_names.contains(key))
            {
                std::cout << std::format("  Unsupported: '{}' | count {}\n", known_group_event_names.at(key), count);
            }
            else
            {
                std::cout << std::format("  Unknown    : GUID {} ; type {} ; version {} | count {}\n", group_to_string(key.group), (int)key.type, key.version, count);
            }
        }
        for(const auto& [key, count] : unprocessed_guid_event_counts)
        {
            if(known_guid_event_names.contains(key))
            {
                std::cout << std::format("  Unsupported: '{}' | count {}\n", known_guid_event_names.at(key), count);
            }
            else
            {
                std::cout << std::format("  Unknown    : GROUP {} ; type {} ; version {} | count {}\n", key.guid.to_string(true), key.type, key.version, count);
            }
        }
    }
}
