
#include <iostream>
#include <ranges>
#include <string>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

struct command_line_args
{
    std::filesystem::path file_path;

    snail::analysis::options  options;
    snail::analysis::path_map module_path_mapper;
};

std::string extract_application_name(std::string_view application_path)
{
    if(application_path.empty()) return "analysis";
    return std::filesystem::path(application_path).filename().string();
}

void print_usage(std::string_view application_path)
{
    std::cout << std::format("Usage: {} <Options> <File>", extract_application_name(application_path)) << "\n"
              << "\n"
              << "File:\n"
              << "  Path to the file to read. Should be a *.diagsession, *.etl or *perf.data\n"
              << "  file.\n"
              << "Options:\n"
              << "  --module-path-map <source> <target>\n"
              << "                   Module file path map. Can be added multiple times.\n";
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

command_line_args parse_command_line(int argc, char* argv[]) // NOLINT(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
{
    const auto application_path = argc > 0 ? std::string_view(argv[0]) : "";

    command_line_args result;
    bool              has_path = false;
    for(int arg_i = 1; arg_i < argc; ++arg_i)
    {
        const auto current_arg = std::string_view(argv[arg_i]);
        if(current_arg == "--module-path-map")
        {
            if(arg_i + 2 >= argc) print_error_and_exit(application_path, "Missing source or target for module path map.");
            const auto source_arg = std::string_view(argv[arg_i + 1]);
            const auto target_arg = std::string_view(argv[arg_i + 2]);
            arg_i += 2;
            result.module_path_mapper.add_rule(std::make_unique<snail::analysis::simple_path_mapper>(
                std::string(source_arg), std::string(target_arg)));
        }
        else
        {
            if(has_path) print_error_and_exit(application_path, "More than one path given.");
            result.file_path = current_arg;
            has_path         = true;
        }
    }

    if(!has_path)
    {
        print_error_and_exit(application_path, "Missing path argument.");
    }

    return result;
}

int main(int argc, char* argv[])
{
    const auto args = parse_command_line(argc, argv);

    auto provider = snail::analysis::make_data_provider(args.file_path.extension(), std::move(args.options), std::move(args.module_path_mapper));
    provider->process(args.file_path);

    const auto& session_info = provider->session_info();
    std::cout << "Session Info:\n";
    std::cout << "-------------\n";
    std::cout << "Command Line:    " << session_info.command_line << "\n";
    std::cout << "Date:            " << session_info.date << "\n";
    std::cout << "Runtime:         " << session_info.runtime << "\n";
    std::cout << "Num. Processes:  " << session_info.number_of_processes << "\n";
    std::cout << "Num. Threads:    " << session_info.number_of_threads << "\n";
    std::cout << "Num. Samples:    " << session_info.number_of_samples << "\n";

    const auto& system_info = provider->system_info();
    std::cout << "\n";
    std::cout << "System Info:\n";
    std::cout << "------------\n";
    std::cout << "Hostname:        " << system_info.hostname << "\n";
    std::cout << "Platform:        " << system_info.platform << "\n";
    std::cout << "CPU Name:        " << system_info.cpu_name << "\n";
    std::cout << "Num. Processors: " << system_info.number_of_processors << "\n";
    std::cout << "Architecture:    " << system_info.architecture << "\n";

    std::cout << "\n";
    std::cout << "Processes:\n";
    std::cout << "----------\n";
    for(const auto process_id : provider->sampling_processes())
    {
        const auto process_info = provider->process_info(process_id);
        std::cout << process_info.name << " (PID " << process_info.os_id << "):\n";
        std::cout << "  Threads:\n";
        for(const auto& thread_info : provider->threads_info(process_id))
        {
            if(thread_info.name)
            {
                std::cout << "  " << *thread_info.name << " (TID " << thread_info.os_id << "):\n";
            }
            else
            {
                std::cout << "    TID " << thread_info.os_id << ":\n";
            }
            if(thread_info.context_switches)
            {
                std::cout << "      Context Switches: " << *thread_info.context_switches << "\n";
            }
            for(const auto& info : thread_info.counters)
            {
                std::cout << "      " << (info.name ? *info.name : "UNKNOWN") << ": " << info.count << "\n";
            }
        }
        const auto stacks_analysis = snail::analysis::analyze_stacks(*provider, process_id, {});
        std::cout << "  Modules:   " << stacks_analysis.all_modules().size() << "\n";
        std::cout << "  Functions: " << stacks_analysis.all_functions().size() << "\n";
        std::cout << "  Files:     " << stacks_analysis.all_files().size() << "\n";
    }

    return EXIT_SUCCESS;
}
