
#include <snail/jsonrpc/stream/unix/unix_domain_socket_streambuf.hpp>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <utility>

using namespace snail::jsonrpc;

unix_domain_socket_streambuf::unix_domain_socket_streambuf() :
    unix_domain_socket_file_(invalid_unix_domain_socket_file_)
{}

unix_domain_socket_streambuf::unix_domain_socket_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode) :
    unix_domain_socket_streambuf()
{
    open(path, mode);
}

unix_domain_socket_streambuf::~unix_domain_socket_streambuf()
{
    close();
}

void unix_domain_socket_streambuf::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    streambuf_base::open(path, mode);

    sockaddr_un socket_address;

    const auto     path_length     = std::strlen(path.c_str());
    constexpr auto max_path_length = sizeof(socket_address.sun_path) - 1; // - 1 because of the null-termination

    if(path_length > max_path_length)
    {
        // FIXME: ERROR
        return;
    }

    unix_domain_socket_file_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(unix_domain_socket_file_ == invalid_unix_domain_socket_file_)
    {
        // FIXME: ERROR
        return;
    }

    std::memset(&socket_address, 0, sizeof(sockaddr_un));

    socket_address.sun_family = AF_UNIX;
    std::strncpy(socket_address.sun_path, path.c_str(), path_length + 1); // + 1 for the null-termination

    if(::connect(unix_domain_socket_file_, (sockaddr*)&socket_address, sizeof(sockaddr_un)) == -1)
    {
        // FIXME: ERROR
        close();
        return;
    }
}

bool unix_domain_socket_streambuf::is_open() const
{
    return unix_domain_socket_file_ != invalid_unix_domain_socket_file_;
}

void unix_domain_socket_streambuf::close()
{
    if(unix_domain_socket_file_ == invalid_unix_domain_socket_file_) return;

    ::close(unix_domain_socket_file_);
    unix_domain_socket_file_ = invalid_unix_domain_socket_file_;
}

std::streamsize unix_domain_socket_streambuf::read(char_type* buffer, std::streamsize bytes_to_read)
{
    const auto read_bytes = ::read(unix_domain_socket_file_, buffer, static_cast<int>(bytes_to_read));
    return static_cast<std::streamsize>(read_bytes);
}

std::streamsize unix_domain_socket_streambuf::write(const char_type* buffer, std::streamsize bytes_to_write)
{
    const auto written_bytes = ::write(unix_domain_socket_file_, buffer, static_cast<int>(bytes_to_write));
    return static_cast<std::streamsize>(written_bytes);
}
