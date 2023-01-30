
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include <Windows.h>

#include <snail/jsonrpc/server.hpp>

#include <snail/jsonrpc/jsonrpc_v2_protocol.hpp>

#include <snail/jsonrpc/stream/windows/pipe_iostream.hpp>

#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/transport/stream_message_reader.hpp>
#include <snail/jsonrpc/transport/stream_message_writer.hpp>

using namespace snail;

namespace {

std::string extract_application_name(std::string_view application_path)
{
    if(application_path.empty()) return "snailserver";
    return std::filesystem::path(application_path).filename().string();
}

void print_usage(std::string_view application_path)
{
    std::cout << std::format("Usage: {} <Options>", extract_application_name(application_path)) << "\n"
              << "\n"
              << "Options:\n"
              << "  --pipe <path>    Path to the pipe to connect to. Cannot be used together with --stdio.\n"
              << "  --stdio          Listen on stdio. Cannot be used together with --pipe.\n";
#if !defined(NDEBUG) && defined(_MSC_VER)
    std::cout << "  --debug          Wait for debugger to attach right after start.\n";
#endif
}

[[noreturn]] void print_usage_and_exit(std::string_view application_path, int exit_code)
{
    print_usage(application_path);
    std::exit(exit_code);
}

[[noreturn]] void print_error_and_exit(std::string_view application_path, std::string_view error)
{
    std::cout << "Error:\n"
              << "  " << error << "\n\n";
    print_usage_and_exit(application_path, EXIT_FAILURE);
}

struct options
{
    std::optional<std::filesystem::path> pipe_name;
    bool                                 stdio = false;
    bool                                 debug = false;
};

options parse_command_line(int argc, char* argv[])
{
    const auto application_path = argc > 0 ? std::string_view(argv[0]) : "";

    options result;
    for(int arg_i = 1; arg_i < argc; ++arg_i)
    {
        const auto current_arg = std::string_view(argv[arg_i]);
        if(current_arg == "--pipe")
        {
            if(argc <= arg_i + 1) print_error_and_exit(application_path, "Missing pipe name.");
            result.pipe_name = argv[arg_i + 1];
        }
        if(current_arg == "--stdio")
        {
            result.stdio = true;
        }
        if(current_arg == "--debug")
        {
            result.debug = true;
        }
    }

    if(result.pipe_name && result.stdio)
    {
        print_error_and_exit(application_path, "Cannot use both, --pipe and --stdio.");
    }
    if(!result.pipe_name && !result.stdio)
    {
        print_error_and_exit(application_path, "Need to specify either --pipe or --stdio.");
    }

    return result;
}

std::unique_ptr<jsonrpc::pipe_iostream> pipe; // FIXME: remove global variable

std::unique_ptr<jsonrpc::message_connection> make_connection(const ::options& options)
{
    if(options.pipe_name)
    {
        std::cout << "  Start listening on pipe " << *options.pipe_name << "\n";
        std::cout.flush();

        pipe = std::make_unique<jsonrpc::pipe_iostream>(*options.pipe_name);
        if(!pipe->is_open())
        {
            std::cout << "Could not open pipe!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        return std::make_unique<jsonrpc::message_connection>(
            std::make_unique<jsonrpc::stream_message_reader>(*pipe),
            std::make_unique<jsonrpc::stream_message_writer>(*pipe));
    }

    if(options.stdio)
    {
        std::cout << "  Start listening stdin\n";
        std::cout.flush();

        return std::make_unique<jsonrpc::message_connection>(
            std::make_unique<jsonrpc::stream_message_reader>(std::cin),
            std::make_unique<jsonrpc::stream_message_writer>(std::cout));
    }

    std::exit(EXIT_FAILURE);
}

void wait_for_debugger()
{
#if !defined(NDEBUG) && defined(_MSC_VER)
    while(!IsDebuggerPresent())
        ;
    DebugBreak();
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    const auto options = parse_command_line(argc, argv);

    std::cout << "Starting Snail server\n";

    if(options.debug) wait_for_debugger();

    jsonrpc::server server(
        make_connection(options),
        std::make_unique<jsonrpc::v2_protocol>());

    server.serve();

    return EXIT_SUCCESS;
}
