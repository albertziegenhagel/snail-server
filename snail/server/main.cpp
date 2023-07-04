
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#if defined(_WIN32)
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <snail/jsonrpc/server.hpp>

#include <snail/jsonrpc/jsonrpc_v2_protocol.hpp>

#if defined(_WIN32)
#    include <snail/jsonrpc/stream/windows/pipe_iostream.hpp>
#endif
#if defined(__unix__)
#    include <snail/jsonrpc/stream/unix/unix_domain_socket_iostream.hpp>
#endif

#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/transport/stream_message_reader.hpp>
#include <snail/jsonrpc/transport/stream_message_writer.hpp>

#include <snail/server/requests/requests.hpp>
#include <snail/server/storage/storage.hpp>

using namespace snail;

namespace {

#if defined(_WIN32)
using socket_stream_type = jsonrpc::pipe_iostream;

inline constexpr std::string_view socket_kind_name = "named pipe";
#elif defined(__unix__)
using socket_stream_type = jsonrpc::unix_domain_socket_iostream;

inline constexpr std::string_view socket_kind_name = "unix domain socket";
#endif

std::string extract_application_name(std::string_view application_path)
{
    if(application_path.empty()) return "snail-server";
    return std::filesystem::path(application_path).filename().string();
}

void print_usage(std::string_view application_path)
{
    std::cout << std::format("Usage: {} <Options>", extract_application_name(application_path)) << "\n"
              << "\n"
              << "Options:\n"
              << "  --help, -h       Show this help text.\n"
              << "  --socket <path>  Path to the socket/pipe to connect to.\n"
              << "                   Uses unix domain sockets on Unix like systems and named pipes\n"
              << "                   on Windows. Cannot be used together with --stdio.\n"
              << "  --pipe   <path>  Alias for --socket.\n"
              << "  --stdio          Connect on stdio. Cannot be used together with --socket (or --pipe).\n";

#if !defined(NDEBUG) && defined(_MSC_VER)
    std::cout << "  --debug          Wait for debugger to attach right after start.\n";
#endif
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
    std::optional<std::filesystem::path> socket_name;
    bool                                 stdio = false;
    bool                                 debug = false;
};

options parse_command_line(int argc, char* argv[]) // NOLINT(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
{
    const auto application_path = argc > 0 ? std::string_view(argv[0]) : "";

    options result;
    bool    help = false;
    for(int arg_i = 1; arg_i < argc; ++arg_i)
    {
        const auto current_arg = std::string_view(argv[arg_i]);
        if(current_arg == "--socket" || current_arg == "--pipe")
        {
            if(argc <= arg_i + 1) print_error_and_exit(application_path, "Missing unix domain socket or named pipe name.");
            result.socket_name = argv[arg_i + 1];
            arg_i += 1;
        }
        else if(current_arg == "--stdio")
        {
            result.stdio = true;
        }
        else if(current_arg == "--debug")
        {
            result.debug = true;
        }
        else if(current_arg == "--help" || current_arg == "-h")
        {
            help = true;
        }
        else
        {
            print_error_and_exit(application_path, std::format("Unknown command line argument: {}", current_arg));
        }
    }

    if(help)
    {
        print_usage_and_exit(application_path, EXIT_SUCCESS);
    }

    if(result.socket_name && result.stdio)
    {
        print_error_and_exit(application_path, "Cannot use both, --socket or --pipe with --stdio.");
    }
    if(!result.socket_name && !result.stdio)
    {
        print_error_and_exit(application_path, "Need to specify either --socket, --pipe or --stdio.");
    }

    return result;
}

std::unique_ptr<socket_stream_type> socket; // FIXME: remove global variable

std::unique_ptr<jsonrpc::message_connection> make_connection(const ::options& options)
{
    if(options.socket_name)
    {
        std::cout << std::format("  Start listening on {} '{}'", socket_kind_name, options.socket_name->string()) << "\n";
        std::cout.flush();

        socket = std::make_unique<socket_stream_type>(*options.socket_name);
        if(!socket->is_open())
        {
            std::cout << std::format("  Failed to open {} '{}'!", socket_kind_name, options.socket_name->string()) << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }
        return std::make_unique<jsonrpc::message_connection>(
            std::make_unique<jsonrpc::stream_message_reader>(*socket),
            std::make_unique<jsonrpc::stream_message_writer>(*socket));
    }

    if(options.stdio)
    {
        std::cout << "  Start listening on stdin\n";
        std::cout.flush();

        return std::make_unique<jsonrpc::message_connection>(
            std::make_unique<jsonrpc::stream_message_reader>(std::cin),
            std::make_unique<jsonrpc::stream_message_writer>(std::cout));
    }

    std::quick_exit(EXIT_FAILURE);
}

void wait_for_debugger()
{
#if !defined(NDEBUG) && defined(_MSC_VER)
    while(IsDebuggerPresent() == 0)
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

    server::storage storage;

    server::register_all(server, storage);

    server.serve();

    return EXIT_SUCCESS;
}
