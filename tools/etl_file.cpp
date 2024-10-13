
#include <snail/etl/etl_file.hpp>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <utf8/cpp17.h>

#include <snail/etl/dispatching_event_observer.hpp>

#include <snail/etl/parser/buffer.hpp>

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

#include <snail/etl/parser/records/snail/profiler.hpp>

#include <snail/common/detail/dump.hpp>

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
              << "  --summary        Print event count summary.\n"
              << "  --dump-headers   Dump event buffers.\n"
              << "  --dump-events    Dump event buffers.\n"
              << "  --dump           Dump event buffers.\n"
              << "  --buffers        Print buffer headers.\n"
              << "  --header         Print header event.\n"
              << "  --config         Print kernel config events.\n"
              << "  --perfinfo       Print kernel perfinfo events (does not include samples).\n"
              << "  --samples        Print kernel sample events.\n"
              << "  --stacks         Print stack events.\n"
              << "  --config-ex      Print XPerf extended config events.\n"
              << "  --vs-diag        Print events from the VS Diagnostics Hub.\n"
              << "  --snail          Print events from the Snail profiling tool.\n"
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

    bool show_events_summary = false;

    bool dump_trace_headers = false;
    bool dump_events        = false;

    bool show_buffers   = false;
    bool show_header    = false;
    bool show_config    = false;
    bool show_perfinfo  = false;
    bool show_samples   = false;
    bool show_stacks    = false;
    bool show_config_ex = false;
    bool show_vs_diag   = false;
    bool show_snail     = false;

    std::optional<std::uint32_t> process_of_interest;
    bool                         all_processes = false;

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
        else if(current_arg == "--summary")
        {
            result.show_events_summary = true;
        }
        else if(current_arg == "--dump-headers")
        {
            result.dump_trace_headers = true;
        }
        else if(current_arg == "--dump-events")
        {
            result.dump_events = true;
        }
        else if(current_arg == "--dump")
        {
            result.dump_trace_headers = true;
            result.dump_events        = true;
        }
        else if(current_arg == "--buffers")
        {
            result.show_buffers = true;
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
        else if(current_arg == "--samples")
        {
            result.show_samples = true;
        }
        else if(current_arg == "--stacks")
        {
            result.show_stacks = true;
        }
        else if(current_arg == "--config-ex")
        {
            result.show_config_ex = true;
        }
        else if(current_arg == "--vs-diag")
        {
            result.show_vs_diag = true;
        }
        else if(current_arg == "--snail")
        {
            result.show_snail = true;
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
                result.process_of_interest.emplace();
                auto chars_result = std::from_chars(next_arg.data(), next_arg.data() + next_arg.size(), result.process_of_interest.value());
                if(chars_result.ec != std::errc{}) print_error_and_exit(application_path, "Invalid argument for --pid.");
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
            print_error_and_exit(application_path, std::format("Multiple files not supported: first was '{}' second is '{}'", etl_file_path->string(), current_arg));
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

constexpr std::string_view get_guid_provider_name(const common::guid& guid)
{
    if(guid == etl::parser::image_id_guid ||
       guid == etl::parser::system_config_ex_guid) return "XPerf";
    if(guid == etl::parser::vs_diagnostics_hub_guid) return "VS";
    if(guid == etl::parser::snail_profiler_guid) return "Snail";
    return "Unknown";
}

template<typename EventType>
void register_known_event_names(std::unordered_map<etl::detail::group_handler_key, std::string>& known_event_names)
{
    // static_assert(is_group_event<EventType>);
    for(const auto& event_identifier : EventType::event_types)
    {
        const auto key = etl::detail::group_handler_key{event_identifier.group, event_identifier.type, EventType::event_version};

        if constexpr(EventType::event_types.size() == 1)
        {
            known_event_names[key] = std::format("Kernel:{}-V{}", EventType::event_name, EventType::event_version);
        }
        else
        {
            known_event_names[key] = std::format("Kernel:{}:{}-V{}", EventType::event_name, event_identifier.name, EventType::event_version);
        }
    }
}

template<typename EventType>
void register_known_event_names(std::unordered_map<etl::detail::guid_handler_key, std::string>& known_event_names)
{
    // static_assert(!is_group_event<EventType>);
    for(const auto& event_identifier : EventType::event_types)
    {
        const auto key           = etl::detail::guid_handler_key{event_identifier.guid, event_identifier.type, EventType::event_version};
        const auto provider_name = get_guid_provider_name(key.guid);

        if constexpr(EventType::event_types.size() == 1)
        {
            known_event_names[key] = std::format("{}:{}-V{}", provider_name, EventType::event_name, EventType::event_version);
        }
        else
        {
            known_event_names[key] = std::format("{}:{}:{}-V{}", provider_name, EventType::event_name, event_identifier.name, EventType::event_version);
        }
    }
}

std::string try_get_known_event_name(const etl::detail::group_handler_key&                                  key,
                                     const std::unordered_map<etl::detail::group_handler_key, std::string>& known_event_names)
{
    const auto iter = known_event_names.find(key);
    if(iter != known_event_names.end()) return iter->second;

    return std::format("Unknown event: group {} ; type {} ; version {}", group_to_string(key.group), (int)key.type, key.version);
}

std::string try_get_known_event_name(const etl::detail::guid_handler_key&                                  key,
                                     const std::unordered_map<etl::detail::guid_handler_key, std::string>& known_event_names)
{
    const auto iter = known_event_names.find(key);
    if(iter != known_event_names.end()) return iter->second;

    return std::format("Unknown event: guid {} ; type {} ; version {}", key.guid.to_string(true), key.type, key.version);
}

class counting_event_observer : public etl::dispatching_event_observer
{
public:
    std::function<void(const etl::etl_file::header_data&, const etl::parser::wmi_buffer_header_view&)> buffer_header_callback_;

    std::unordered_map<etl::detail::group_handler_key, std::size_t> handled_group_event_counts;
    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  handled_guid_event_counts;
    std::unordered_map<etl::detail::group_handler_key, std::size_t> unknown_group_event_counts;
    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  unknown_guid_event_counts;

    std::unordered_map<etl::detail::group_handler_key, std::string> known_group_event_names;
    std::unordered_map<etl::detail::guid_handler_key, std::string>  known_guid_event_names;

    std::string_view current_event_name; // hacky way to sneak the event name into the handler routines.

    virtual void handle_buffer(const etl::etl_file::header_data&          file_header,
                               const etl::parser::wmi_buffer_header_view& buffer_header) override
    {
        if(!buffer_header_callback_) return;
        buffer_header_callback_(file_header, buffer_header);
    }

protected:
    virtual void pre_handle([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                            const etl::detail::group_handler_key&               key,
                            [[maybe_unused]] const etl::any_group_trace_header& trace_header,
                            [[maybe_unused]] std::span<const std::byte>         user_data,
                            bool                                                has_known_handler) override
    {
        if(has_known_handler)
        {
            ++handled_group_event_counts[key];
            current_event_name = known_group_event_names.at(key);
        }
        else ++unknown_group_event_counts[key];
    }

    virtual void pre_handle([[maybe_unused]] const etl::etl_file::header_data& file_header,
                            const etl::detail::guid_handler_key&               key,
                            [[maybe_unused]] const etl::any_guid_trace_header& trace_header,
                            [[maybe_unused]] std::span<const std::byte>        user_data,
                            bool                                               has_known_handler) override
    {
        if(has_known_handler)
        {
            ++handled_guid_event_counts[key];
            current_event_name = known_guid_event_names.at(key);
        }
        else ++unknown_guid_event_counts[key];
    }
};

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

    std::cout << std::format("  start_time:           {}\n", file.header().start_time);
    std::cout << std::format("  end_time:             {}\n", file.header().end_time);
    std::cout << std::format("  start_time_qpc_ticks: {}\n", file.header().start_time_qpc_ticks);
    std::cout << std::format("  qpc_frequency:        {}\n", file.header().qpc_frequency);
    std::cout << std::format("  pointer_size:         {}\n", file.header().pointer_size);
    std::cout << std::format("  number_of_processors: {}\n", file.header().number_of_processors);
    std::cout << std::format("  number_of_buffers:    {}\n", file.header().number_of_buffers);
    std::cout << std::format("  buffer_size:          {}\n", file.header().buffer_size);
    std::cout << std::format("  log_file_mode:        {}\n", file.header().log_file_mode.data().to_string());
    std::cout << std::format("  compression_format:   {}\n", (int)file.header().compression_format);

    counting_event_observer observer;

    std::size_t                                    sample_count = 0;
    std::unordered_map<std::uint16_t, std::size_t> pmc_sample_count;
    std::size_t                                    stack_count = 0;

    std::unordered_map<std::uint32_t, std::uint32_t> thread_to_process;

    // Pre-populate with names of events that we know, but which we do not support (yet).
    // Most of those events should not be important for the functionality of a performance profiler,
    // so we will probably never implement them.
    observer.known_group_event_names = std::unordered_map<etl::detail::group_handler_key, std::string>{
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 8, 2},   "Kernel:EventTrace-RDComplete-V2"      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 64, 2},  "Kernel:EventTrace-DbgIdRSDS-V2"       },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 66, 2},  "Kernel:EventTrace-BuildInfo-V2"       },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 11, 2}, "Kernel:Process-Terminate-V2"          },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 39, 5}, "Kernel:Process-TypeGroup1:Defunct-V5" },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 33, 2},   "Kernel:Image-KernelImageBase-V2"      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 34, 2},   "Kernel:Image-HypercallPage-V2"        },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 13, 2},  "Kernel:SystemConfig-Nic-V2"           },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 14, 2},  "Kernel:SystemConfig-Video-V2"         },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 15, 3},  "Kernel:SystemConfig-Services-V3"      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 16, 2},  "Kernel:SystemConfig-Power-V2"         },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 21, 3},  "Kernel:SystemConfig-Irq-V3"           },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 23, 2},  "Kernel:SystemConfig-IDEChannel-V2"    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 24, 2},  "Kernel:SystemConfig-NumaNode-V2"      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 25, 2},  "Kernel:SystemConfig-Platform-V2"      },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 26, 2},  "Kernel:SystemConfig-ProcGroup-V2"     },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 27, 2},  "Kernel:SystemConfig-ProcNumber-V2"    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 28, 2},  "Kernel:SystemConfig-Dpi-V2"           },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 29, 2},  "Kernel:SystemConfig-CodeIntegrity-V2" },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 30, 2},  "Kernel:SystemConfig-TelemetryInfo-V2" },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 31, 2},  "Kernel:SystemConfig-Defrag-V2"        },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 33, 2},  "Kernel:SystemConfig-DeviceFamily-V2"  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 34, 2},  "Kernel:SystemConfig-FlightIds-V2"     },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 35, 2},  "Kernel:SystemConfig-Processors-V2"    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 36, 2},  "Kernel:SystemConfig-Virtualization-V2"},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 37, 2},  "Kernel:SystemConfig-Boot-V2"          },
    };
    observer.known_guid_event_names = std::unordered_map<etl::detail::guid_handler_key, std::string>{
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 32, 2},         "XPerf:ImageId-DbgIdNone-V2"           },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 34, 2},         "XPerf:ImageId-34?-V2"                 },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 38, 2},         "XPerf:ImageId-DbgIdPPdb-V2"           },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 39, 1},         "XPerf:ImageId-39?-V1"                 }, // Error / Embedded PDB?
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 40, 1},         "XPerf:ImageId-DbgIdDeterm-V1"         },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 41, 1},         "XPerf:ImageId-41?-V1"                 }, // Error / Embedded PDB?
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 64, 0},         "XPerf:ImageId-DbgIdFileVersion-V0"    },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 34, 0}, "XPerf:SystemConfigEx-UnknownVolume-V0"},
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 36, 0}, "XPerf:SystemConfigEx-36?-V0"          }, // NetworkInterface?
    };

    // Unknown events
    {
        observer.register_unknown_event(
            [&options]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                       [[maybe_unused]] const etl::any_guid_trace_header& header,
                       const std::span<const std::byte>&                  event_data)
            {
                if(!options.only.empty()) return;
                if(options.dump_events) common::detail::dump_buffer(event_data, 0, event_data.size(), "event");
            });
        observer.register_unknown_event(
            [&options]([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                       [[maybe_unused]] const etl::any_group_trace_header& header,
                       const std::span<const std::byte>&                   event_data)
            {
                if(!options.only.empty()) return;
                if(options.dump_events) common::detail::dump_buffer(event_data, 0, event_data.size(), "event");
            });
    }

    // Buffer
    observer.buffer_header_callback_ = [&options]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                                  const etl::parser::wmi_buffer_header_view&         buffer_header)
    {
        if(!options.show_buffers) return;

        std::cout << "Buffer Header:\n";

        std::cout << std::format("    buffer_size:      {}\n", buffer_header.wnode().buffer_size());
        std::cout << std::format("    saved_offset:     {}\n", buffer_header.wnode().saved_offset());
        std::cout << std::format("    current_offset:   {}\n", buffer_header.wnode().current_offset());
        std::cout << std::format("    reference_count:  {}\n", buffer_header.wnode().reference_count());
        std::cout << std::format("    timestamp:        {}\n", buffer_header.wnode().timestamp());
        std::cout << std::format("    sequence_number:  {}\n", buffer_header.wnode().sequence_number());
        std::cout << std::format("    clock:            {}\n", buffer_header.wnode().clock());
        std::cout << std::format("    processor_index:  {}\n", buffer_header.wnode().client_context().processor_index());
        std::cout << std::format("    logger_id:        {}\n", buffer_header.wnode().client_context().logger_id());
        std::cout << std::format("    state:            {}\n", buffer_header.wnode().state());
        std::cout << std::format("    offset:           {}\n", buffer_header.offset());
        std::cout << std::format("    buffer_flag:      {}\n", buffer_header.buffer_flag().data().to_string());
        std::cout << std::format("    buffer_type:      {}\n", (int)buffer_header.buffer_type());
        std::cout << std::format("    start_time:       {}\n", buffer_header.start_time());
        std::cout << std::format("    start_perf_clock: {}\n", buffer_header.start_perf_clock());
    };

    // Kernel: header
    {
        register_known_event_names<etl::parser::event_trace_v2_header_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::event_trace_v2_header_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                                  [[maybe_unused]] const etl::common_trace_header&     header,
                                  const etl::parser::event_trace_v2_header_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_header) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}:\n", header.timestamp, observer.current_event_name);
                std::cout << std::format("    buffer_size:           {}\n", event.buffer_size());
                std::cout << std::format("    os_version_major:      {}\n", event.os_version_major());
                std::cout << std::format("    os_version_minor:      {}\n", event.os_version_minor());
                std::cout << std::format("    sp_version_major:      {}\n", event.sp_version_major());
                std::cout << std::format("    sp_version_minor:      {}\n", event.sp_version_minor());
                std::cout << std::format("    build_number:          {}\n", event.provider_version());
                std::cout << std::format("    number_of_processors:  {}\n", event.number_of_processors());
                std::cout << std::format("    end_time:              {}\n", event.end_time());
                std::cout << std::format("    timer_resolution:      {}\n", event.timer_resolution());
                std::cout << std::format("    max_file_size:         {}\n", event.max_file_size());
                std::cout << std::format("    log_file_mode:         {}\n", event.log_file_mode().data().to_string());
                std::cout << std::format("    buffers_written:       {}\n", event.buffers_written());
                std::cout << std::format("    start_buffers:         {}\n", event.start_buffers());
                std::cout << std::format("    pointer_size:          {}\n", event.pointer_size());
                std::cout << std::format("    events_lost:           {}\n", event.events_lost());
                std::cout << std::format("    logger_name:           {}\n", event.logger_name());
                std::cout << std::format("    log_file_name:         {}\n", event.log_file_name());
                std::cout << std::format("    time_zone.bias:        {}\n", event.time_zone_information().bias());
                std::cout << std::format("    boot_time:             {}\n", event.boot_time());
                std::cout << std::format("    perf_freq:             {}\n", event.perf_freq());
                std::cout << std::format("    start_time:            {}\n", event.start_time());
                std::cout << std::format("    reserved_flags:        {}\n", event.reserved_flags());
                std::cout << std::format("    buffers_lost:          {}\n", event.buffers_lost());
                std::cout << std::format("    session_name:          '{}'\n", utf8::utf16to8(event.session_name()));
                std::cout << std::format("    file_name:             '{}'\n", utf8::utf16to8(event.file_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel: config
    {
        register_known_event_names<etl::parser::system_config_v3_cpu_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::system_config_v3_cpu_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                                  [[maybe_unused]] const etl::common_trace_header&    header,
                                  const etl::parser::system_config_v3_cpu_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: computer_name '{}' architecture {}...\n", header.timestamp, observer.current_event_name, utf8::utf16to8(event.computer_name()), event.processor_architecture());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::system_config_v2_physical_disk_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::system_config_v2_physical_disk_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&            file_header,
                                  [[maybe_unused]] const etl::common_trace_header&              header,
                                  const etl::parser::system_config_v2_physical_disk_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: disk_number {} partition_count {} ...\n", header.timestamp, observer.current_event_name, event.disk_number(), event.partition_count());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::system_config_v2_logical_disk_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::system_config_v2_logical_disk_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&           file_header,
                                  [[maybe_unused]] const etl::common_trace_header&             header,
                                  const etl::parser::system_config_v2_logical_disk_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: disk_number {} partition_number {} drive_letter {} ...\n", header.timestamp, observer.current_event_name, event.disk_number(), event.partition_number(), utf8::utf16to8(event.drive_letter()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::system_config_v5_pnp_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::system_config_v5_pnp_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&  file_header,
                                  [[maybe_unused]] const etl::common_trace_header&    header,
                                  const etl::parser::system_config_v5_pnp_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: device_description '{}' friendly_name '{}' ...\n", header.timestamp, observer.current_event_name, utf8::utf16to8(event.device_description()), utf8::utf16to8(event.friendly_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel: perfinfo/stack
    {
        register_known_event_names<etl::parser::stackwalk_v2_stack_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::stackwalk_v2_stack_event_view>(
            [&options, &observer, &stack_count]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                                [[maybe_unused]] const etl::common_trace_header&   header,
                                                const etl::parser::stackwalk_v2_stack_event_view&  event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                ++stack_count;

                if(!options.show_stacks) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: event time {} pid {} tid {} count {}\n", header.timestamp, observer.current_event_name, event.event_timestamp(), event.process_id(), event.thread_id(), event.stack_size());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::stackwalk_v2_key_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::stackwalk_v2_key_event_view>(
            [&options, &observer, &stack_count]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                                [[maybe_unused]] const etl::common_trace_header&   header,
                                                const etl::parser::stackwalk_v2_key_event_view&    event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                ++stack_count;

                if(!options.show_stacks) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: event time {} pid {} tid {} key {:#018x}\n", header.timestamp, observer.current_event_name, event.event_timestamp(), event.process_id(), event.thread_id(), event.stack_key());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::stackwalk_v2_type_group1_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::stackwalk_v2_type_group1_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&      file_header,
                                  [[maybe_unused]] const etl::common_trace_header&        header,
                                  const etl::parser::stackwalk_v2_type_group1_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.all_processes) return;

                if(!options.show_stacks) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: key {:#018x} count {}\n", header.timestamp, observer.current_event_name, event.stack_key(), event.stack_size());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::perfinfo_v2_sampled_profile_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>(
            [&options, &observer, &sample_count, &thread_to_process]([[maybe_unused]] const etl::etl_file::header_data&                          file_header,
                                                                     [[maybe_unused]] const etl::common_trace_header&                            header,
                                                                     [[maybe_unused]] const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                auto process_id = thread_to_process.find(event.thread_id());
                if(!options.all_processes && (process_id != thread_to_process.end() && process_id->second != options.process_of_interest)) return;

                ++sample_count;

                if(!options.show_samples) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: thread {} count {} ip {:#018x}\n", header.timestamp, observer.current_event_name, event.thread_id(), event.count(), event.instruction_pointer());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::perfinfo_v2_pmc_counter_profile_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::perfinfo_v2_pmc_counter_profile_event_view>(
            [&options, &observer, &pmc_sample_count, &thread_to_process]([[maybe_unused]] const etl::etl_file::header_data&             file_header,
                                                                         [[maybe_unused]] const etl::common_trace_header&               header,
                                                                         const etl::parser::perfinfo_v2_pmc_counter_profile_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                auto process_id = thread_to_process.find(event.thread_id());
                if(!options.all_processes && (process_id != thread_to_process.end() && process_id->second != options.process_of_interest)) return;

                auto iter = pmc_sample_count.find(event.profile_source());
                if(iter == pmc_sample_count.end())
                {
                    pmc_sample_count[event.profile_source()] = 1;
                }
                else
                {
                    ++iter->second;
                }

                if(!options.show_samples) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: thread {} source {} ip {:#018x}\n", header.timestamp, observer.current_event_name, event.thread_id(), event.profile_source(), event.instruction_pointer());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::perfinfo_v3_sampled_profile_interval_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::perfinfo_v3_sampled_profile_interval_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&                  file_header,
                                  [[maybe_unused]] const etl::common_trace_header&                    header,
                                  const etl::parser::perfinfo_v3_sampled_profile_interval_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_perfinfo) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: source {} new-interval {} old-interval {} source-name '{}'\n", header.timestamp, observer.current_event_name, event.source(), event.new_interval(), event.old_interval(), utf8::utf16to8(event.source_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::perfinfo_v2_pmc_counter_config_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::perfinfo_v2_pmc_counter_config_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&            file_header,
                                  [[maybe_unused]] const etl::common_trace_header&              header,
                                  const etl::parser::perfinfo_v2_pmc_counter_config_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_perfinfo) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::string names;
                for(std::uint32_t i = 0; i < event.counter_count(); ++i)
                {
                    if(i > 0) names.push_back(',');
                    names += std::format("'{}'", utf8::utf16to8(event.counter_name(i)));
                }

                std::cout << std::format("@{} {:30}: count {} names {}\n", header.timestamp, observer.current_event_name, event.counter_count(), names);

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel: process/thread
    {
        register_known_event_names<etl::parser::process_v4_type_group1_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::process_v4_type_group1_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&    file_header,
                                  const etl::common_trace_header&                       header,
                                  const etl::parser::process_v4_type_group1_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} filename '{}' cmd '{}' unique {} flags {} ...\n", header.timestamp, observer.current_event_name, process_id, event.image_filename(), utf8::utf16to8(event.command_line()), event.unique_process_key(), event.flags());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::thread_v3_type_group1_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::thread_v3_type_group1_event_view>(
            [&options, &observer, &thread_to_process]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                                                      const etl::common_trace_header&                      header,
                                                      const etl::parser::thread_v3_type_group1_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id                = event.process_id();
                thread_to_process[event.thread_id()] = process_id;

                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} name '{}'...\n", header.timestamp, observer.current_event_name, process_id, event.thread_id(), event.thread_name() ? utf8::utf16to8(*event.thread_name()) : "<none>");

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::thread_v4_type_group1_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::thread_v4_type_group1_event_view>(
            [&options, &observer, &thread_to_process]([[maybe_unused]] const etl::etl_file::header_data&   file_header,
                                                      const etl::common_trace_header&                      header,
                                                      const etl::parser::thread_v4_type_group1_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id                = event.process_id();
                thread_to_process[event.thread_id()] = process_id;

                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} name {}...\n", header.timestamp, observer.current_event_name, process_id, event.thread_id(), utf8::utf16to8(event.thread_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::thread_v2_set_name_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::thread_v2_set_name_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                  const etl::common_trace_header&                    header,
                                  const etl::parser::thread_v2_set_name_event_view&  event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} tid {} name '{}'\n", header.timestamp, observer.current_event_name, process_id, event.thread_id(), utf8::utf16to8(event.thread_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });

        register_known_event_names<etl::parser::thread_v4_context_switch_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::thread_v4_context_switch_event_view>(
            [&options, &observer, &thread_to_process]([[maybe_unused]] const etl::etl_file::header_data&      file_header,
                                                      const etl::any_group_trace_header&                      header_variant,
                                                      const etl::parser::thread_v4_context_switch_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto old_thread_id  = event.old_thread_id();
                auto       old_process_id = thread_to_process.find(old_thread_id);
                const auto new_thread_id  = event.new_thread_id();
                auto       new_process_id = thread_to_process.find(new_thread_id);

                const auto found_old_process = old_process_id != thread_to_process.end();
                const auto found_new_process = new_process_id != thread_to_process.end();

                const auto old_process_of_interest = options.process_of_interest && found_old_process && old_process_id->second == *options.process_of_interest;
                const auto new_process_of_interest = options.process_of_interest && found_new_process && new_process_id->second == *options.process_of_interest;

                if(!options.all_processes && !old_process_of_interest && !new_process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                const auto header = etl::make_common_trace_header(header_variant);

                std::cout << std::format("@{} {:30}: old tid {} new tid {}", header.timestamp, observer.current_event_name, old_thread_id, new_thread_id);
                const auto* perfinfo_header = std::get_if<etl::parser::perfinfo_trace_header_view>(&header_variant);
                if(perfinfo_header)
                {
                    if(perfinfo_header->has_ext_pebs())
                    {
                        std::cout << std::format(" ext pebs {:#018x}", perfinfo_header->ext_pebs());
                    }
                    for(std::size_t counter_index = 0; counter_index < perfinfo_header->ext_pmc_count(); ++counter_index)
                    {
                        std::cout << std::format(" ext pmc {} {}", counter_index, perfinfo_header->ext_pmc(counter_index));
                    }
                }
                std::cout << "\n";

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel: image
    {
        register_known_event_names<etl::parser::image_v3_load_event_view>(observer.known_group_event_names);
        observer.register_event<etl::parser::image_v3_load_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                  const etl::common_trace_header&                    header,
                                  const etl::parser::image_v3_load_event_view&       event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} name '{}' image-base {:#0x} checksum {} time-date-stamp {}\n", header.timestamp, observer.current_event_name, process_id, utf8::utf16to8(event.file_name()), event.image_base(), event.image_checksum(), event.time_date_stamp());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel-trace-control (Xperf): image
    {
        register_known_event_names<etl::parser::image_id_v2_info_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::image_id_v2_info_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                                  const etl::common_trace_header&                    header,
                                  const etl::parser::image_id_v2_info_event_view&    event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} name '{}' image_base {:#0x} time_stamp {}\n", header.timestamp, observer.current_event_name, process_id, utf8::utf16to8(event.original_file_name()), event.image_base(), event.time_date_stamp());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::image_id_v2_dbg_id_pdb_info_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::image_id_v2_dbg_id_pdb_info_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&         file_header,
                                  const etl::common_trace_header&                            header,
                                  const etl::parser::image_id_v2_dbg_id_pdb_info_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                const auto process_id = event.process_id();
                if(!options.all_processes && process_id != options.process_of_interest) return;

                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} image-base {:#0x} guid {} age {} pdb '{}'\n", header.timestamp, observer.current_event_name, process_id, event.image_base(), event.guid().instantiate().to_string(), event.age(), event.pdb_file_name());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Kernel-trace-control (Xperf): system-config-ex
    {
        register_known_event_names<etl::parser::system_config_ex_v0_build_info_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::system_config_ex_v0_build_info_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&            file_header,
                                  const etl::common_trace_header&                               header,
                                  const etl::parser::system_config_ex_v0_build_info_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config_ex) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: install-date {} build-lab '{}' product-name '{}'\n", header.timestamp, observer.current_event_name, event.install_date(), utf8::utf16to8(event.build_lab()), utf8::utf16to8(event.product_name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::system_config_ex_v0_system_paths_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::system_config_ex_v0_system_paths_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&              file_header,
                                  const etl::common_trace_header&                                 header,
                                  const etl::parser::system_config_ex_v0_system_paths_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config_ex) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: sys-dir '{}' sys-win-dir '{}'\n", header.timestamp, observer.current_event_name, utf8::utf16to8(event.system_directory()), utf8::utf16to8(event.system_windows_directory()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::system_config_ex_v0_volume_mapping_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::system_config_ex_v0_volume_mapping_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&                file_header,
                                  const etl::common_trace_header&                                   header,
                                  const etl::parser::system_config_ex_v0_volume_mapping_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_config_ex) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: nt-path '{}' dos-path '{}'\n", header.timestamp, observer.current_event_name, utf8::utf16to8(event.nt_path()), utf8::utf16to8(event.dos_path()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // VS-Diagnostics-Hub
    {
        register_known_event_names<etl::parser::vs_diagnostics_hub_target_profiling_started_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::vs_diagnostics_hub_target_profiling_started_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&                         file_header,
                                  const etl::common_trace_header&                                            header,
                                  const etl::parser::vs_diagnostics_hub_target_profiling_started_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_vs_diag) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} timestamp {}\n", header.timestamp, observer.current_event_name, event.process_id(), event.timestamp());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::vs_diagnostics_hub_target_profiling_stopped_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::vs_diagnostics_hub_target_profiling_stopped_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&                         file_header,
                                  const etl::common_trace_header&                                            header,
                                  const etl::parser::vs_diagnostics_hub_target_profiling_stopped_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_vs_diag) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {} timestamp {}\n", header.timestamp, observer.current_event_name, event.process_id(), event.timestamp());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::vs_diagnostics_hub_machine_info_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::vs_diagnostics_hub_machine_info_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&             file_header,
                                  [[maybe_unused]] const etl::common_trace_header&               header,
                                  const etl::parser::vs_diagnostics_hub_machine_info_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_vs_diag) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: architecture {} os '{}' name '{}'\n", header.timestamp, observer.current_event_name, (int)event.architecture(), utf8::utf16to8(event.os_description()), utf8::utf16to8(event.name()));

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
        register_known_event_names<etl::parser::vs_diagnostics_hub_counter_info_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::vs_diagnostics_hub_counter_info_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&             file_header,
                                  [[maybe_unused]] const etl::common_trace_header&               header,
                                  const etl::parser::vs_diagnostics_hub_counter_info_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_vs_diag) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: Count {} timestamp {} value {}\n", header.timestamp, observer.current_event_name, (int)event.counter(), event.timestamp(), event.value());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    // Snail-Profiler
    {
        register_known_event_names<etl::parser::snail_profiler_profile_target_event_view>(observer.known_guid_event_names);
        observer.register_event<etl::parser::snail_profiler_profile_target_event_view>(
            [&options, &observer]([[maybe_unused]] const etl::etl_file::header_data&           file_header,
                                  const etl::common_trace_header&                              header,
                                  const etl::parser::snail_profiler_profile_target_event_view& event)
            {
                assert(event.dynamic_size() == event.buffer().size());

                if(!options.show_snail) return;
                if(should_ignore(options, observer.current_event_name)) return;

                std::cout << std::format("@{} {:30}: pid {}\n", header.timestamp, observer.current_event_name, event.process_id());

                if(options.dump_trace_headers) common::detail::dump_buffer(header.buffer, 0, header.buffer.size(), "header");
                if(options.dump_events) common::detail::dump_buffer(event.buffer(), 0, event.buffer().size(), "event");
            });
    }

    std::cout << "\n";
    try
    {
        file.process(observer);
    }
    catch(const std::exception& e)
    {
        std::cerr << std::format("Failed to process ETL file: {}\n", e.what());
        return EXIT_FAILURE;
    }

    std::cout << "\n";
    std::cout << "Number of samples:\n";
    std::cout << std::format("  Regular Profile: {}\n", sample_count);
    for(const auto& [source, count] : pmc_sample_count)
    {
        std::cout << std::format("  PMC profile {}: {}\n", source, count);
    }
    std::cout << std::format("Number of stacks:  {}\n", stack_count);

    if(options.show_events_summary)
    {
        std::cout << "\n";
        std::cout << "Supported Events:\n";
        std::vector<std::pair<std::string, std::size_t>> event_counts;
        for(const auto& [key, count] : observer.handled_group_event_counts)
        {
            event_counts.emplace_back(try_get_known_event_name(key, observer.known_group_event_names), count);
        }
        for(const auto& [key, count] : observer.handled_guid_event_counts)
        {
            event_counts.emplace_back(try_get_known_event_name(key, observer.known_guid_event_names), count);
        }
        std::ranges::sort(event_counts);
        for(const auto& [name, count] : event_counts)
        {
            std::cout << std::format("  {:60} : {}\n", name, count);
        }
        std::cout << "Unsupported Events:\n";
        event_counts.clear();
        for(const auto& [key, count] : observer.unknown_group_event_counts)
        {
            event_counts.emplace_back(try_get_known_event_name(key, observer.known_group_event_names), count);
        }
        for(const auto& [key, count] : observer.unknown_guid_event_counts)
        {
            event_counts.emplace_back(try_get_known_event_name(key, observer.known_guid_event_names), count);
        }
        std::ranges::sort(event_counts);
        for(const auto& [name, count] : event_counts)
        {
            std::cout << std::format("  {:60} : {}\n", name, count);
        }
    }
}
