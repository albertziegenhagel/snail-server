
#include <format>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <snail/perf_data/dispatching_event_observer.hpp>
#include <snail/perf_data/metadata.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/detail/file_header.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;

namespace {

std::string extract_application_name(std::string_view application_path)
{
    if(application_path.empty()) return "perf_data_file";
    return std::filesystem::path(application_path).filename().string();
}

void print_usage(std::string_view application_path)
{
    std::cout << std::format("Usage: {} <Options> <File>", extract_application_name(application_path)) << "\n"
              << "\n"
              << "File:\n"
              << "  Path to the perf.data file to be read.\n"
              << "Options:\n"
              << "  --help, -h       Show this help text.\n";
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
};

options parse_command_line(int argc, char* argv[]) // NOLINT(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
{
    const auto application_path = argc > 0 ? std::string_view(argv[0]) : "";

    options                              result;
    bool                                 help = false;
    std::optional<std::filesystem::path> file_path;
    for(int arg_i = 1; arg_i < argc; ++arg_i)
    {
        const auto current_arg = std::string_view(argv[arg_i]);
        if(current_arg == "--help" || current_arg == "-h")
        {
            help = true;
        }
        else if(current_arg.starts_with("-"))
        {
            print_error_and_exit(application_path, std::format("Unknown command line argument: {}", current_arg));
        }
        else if(file_path != std::nullopt)
        {
            print_error_and_exit(application_path, std::format("Multiple files not supported: first was '{}' second is '{}'", file_path->string(), current_arg));
        }
        else
        {
            file_path = current_arg;
        }
    }

    if(help)
    {
        print_usage_and_exit(application_path, EXIT_SUCCESS);
    }

    if(!file_path)
    {
        print_error_and_exit(application_path, "Missing perf.data file.");
    }
    result.file_path = *file_path;

    return result;
}

} // namespace

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    const auto options = parse_command_line(argc, argv);

    perf_data::perf_data_file file;

    try
    {
        std::cout << std::format("File: {}\n", options.file_path.string());
        file.open(options.file_path);
    }
    catch(const std::exception& e)
    {
        std::cerr << std::format("Failed to open perf.data file: {}\n", e.what());
        return EXIT_FAILURE;
    }

    perf_data::dispatching_event_observer observer;

    struct samples_range
    {
        std::size_t count = 0;

        std::uint64_t first_time = std::numeric_limits<std::uint64_t>::max();
        std::uint64_t last_time  = std::numeric_limits<std::uint64_t>::min();
    };

    std::unordered_map<std::optional<std::uint64_t>, samples_range> round_samples;
    std::unordered_map<std::optional<std::uint64_t>, samples_range> total_samples;

    const auto flush_samples = [&round_samples, &total_samples]()
    {
        for(auto& [id, round_info] : round_samples)
        {
            if(round_info.count == 0) continue;
            const auto id_str = id ? std::to_string(*id) : "null";
            std::cout << std::format("SAMPLES @{}-{} id {} count {}", round_info.first_time, round_info.last_time, id_str, round_info.count) << "\n";

            auto& total_info = total_samples[id];
            total_info.count += round_info.count;
            total_info.first_time = std::min(total_info.first_time, round_info.first_time);
            total_info.last_time  = std::max(total_info.last_time, round_info.last_time);

            round_info.count      = 0;
            round_info.first_time = std::numeric_limits<std::uint64_t>::max();
            round_info.last_time  = std::numeric_limits<std::uint64_t>::min();
        }
    };

    observer.register_event<perf_data::parser::id_index_event_view>(
        [&](const perf_data::parser::id_index_event_view& event)
        {
            flush_samples();
            std::cout << std::format("ID INDEX; nr {}:", event.nr()) << "\n";
            const auto nr = event.nr();
            for(std::uint64_t i = 0; i < nr; ++i)
            {
                const auto entry = event.entry(i);
                std::cout << std::format("  cpu {} tid {}: id {} idx {}", entry.cpu(), entry.tid(), entry.id(), entry.idx()) << "\n";
            }
        });
    observer.register_event<perf_data::parser::thread_map_event_view>(
        [&](const perf_data::parser::thread_map_event_view& event)
        {
            flush_samples();
            std::cout << std::format("THREAD MAP; nr {}:", event.nr()) << "\n";
            const auto nr = event.nr();
            for(std::uint64_t i = 0; i < nr; ++i)
            {
                const auto entry = event.entry(i);
                std::cout << std::format("  pid {} comm '{}'", entry.pid(), entry.comm()) << "\n";
            }
        });
    observer.register_event<perf_data::parser::cpu_map_event_view>(
        [&](const perf_data::parser::cpu_map_event_view& event)
        {
            flush_samples();
            switch(event.type())
            {
            case perf_data::parser::cpu_map_type::cpus:
            {
                std::cout << std::format("CPU MAP (CPUS)") << "\n";
                [[maybe_unused]] const auto data  = event.cpus_data();
                [[maybe_unused]] const auto count = data.nr();
                break;
            }
            case perf_data::parser::cpu_map_type::mask:
            {
                const auto data      = event.mask_data();
                const auto nr        = data.nr();
                const auto long_size = data.long_size();
                std::cout << std::format("CPU MAP (MASK) nr {} long size {}", nr, long_size) << "\n";
                for(std::uint64_t i = 0; i < nr; ++i)
                {
                    std::cout << std::format("  mask {}", long_size == 4 ? data.mask_32(i) : data.mask_64(i)) << "\n";
                }
                break;
            }
            case perf_data::parser::cpu_map_type::range_cpus:
            {
                std::cout << std::format("CPU MAP (RANGE)") << "\n";
                [[maybe_unused]] const auto data      = event.range_cpus_data();
                [[maybe_unused]] const auto any_cpu   = data.any_cpu();
                [[maybe_unused]] const auto start_cpu = data.start_cpu();
                [[maybe_unused]] const auto end_cpu   = data.end_cpu();
                break;
            }
            }
        });
    observer.register_event<perf_data::parser::comm_event_view>(
        [&](const perf_data::parser::comm_event_view& event)
        {
            flush_samples();
            std::cout << std::format("COMM @{} ({},{}): {}", *event.sample_id().time, event.pid(), event.tid(), event.comm()) << "\n";
        });
    observer.register_event<perf_data::parser::mmap2_event_view>(
        [&](const perf_data::parser::mmap2_event_view& event)
        {
            flush_samples();
            std::cout << std::format("MMAP2 @{} ({},{}): {} @{:#018x} + {:#018x} ; {:#18x}", *event.sample_id().time, event.pid(), event.tid(), event.filename(), event.addr(), event.len(), event.pgoff()) << "\n";
        });
    observer.register_event<perf_data::parser::fork_event_view>(
        [&](const perf_data::parser::fork_event_view& event)
        {
            flush_samples();
            std::cout << std::format("FORK @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::exit_event_view>(
        [&](const perf_data::parser::exit_event_view& event)
        {
            flush_samples();
            std::cout << std::format("EXIT @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::finished_round_event_view>(
        [&](const perf_data::parser::finished_round_event_view& /*event*/)
        {
            flush_samples();
            std::cout << std::format("FINISHED ROUND") << "\n";
        });
    observer.register_event<perf_data::parser::finished_init_event_view>(
        [&](const perf_data::parser::finished_init_event_view& /*event*/)
        {
            flush_samples();
            std::cout << std::format("FINISHED INIT") << "\n";
        });

    observer.register_event<perf_data::parser::sample_event>(
        [&](const perf_data::parser::sample_event& event)
        {
            auto& info = round_samples[event.id];
            ++info.count;
            info.first_time = std::min(info.first_time, *event.time);
            info.last_time  = std::max(info.last_time, *event.time);
        });

    file.process(observer);

    std::cout << "\n";
    std::cout << "Total samples:\n";
    for(const auto& [id, info] : total_samples)
    {
        auto id_name = id ? std::to_string(*id) : "null";
        if(id)
        {
            for(const auto& desc : file.metadata().event_desc)
            {
                if(std::ranges::find(desc.ids, *id) != desc.ids.end())
                {
                    id_name = std::format("{} ({})", desc.event_string, *id);
                    break;
                }
            }
        }
        std::cout << std::format("  id {} @{}-{} count {}", id_name, info.first_time, info.last_time, info.count) << "\n";
    }
}
