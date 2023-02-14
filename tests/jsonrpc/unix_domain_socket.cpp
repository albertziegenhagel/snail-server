#include <gtest/gtest.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>

#include <snail/jsonrpc/stream/unix/unix_domain_socket_iostream.hpp>

using namespace snail::jsonrpc;

struct unix_domain_socket
{
    unix_domain_socket(const std::filesystem::path& path)
    {
        file_descriptor_ = ::socket(AF_UNIX, SOCK_STREAM, 0);

        path_ = path;

        sockaddr_un socket_address;

        std::memset(&socket_address, 0, sizeof(sockaddr_un));
        socket_address.sun_family = AF_UNIX;
        std::strncpy(socket_address.sun_path, path.c_str(), path.string().size() + 1); // + 1 for the null-termination

        ::unlink(path.c_str());
        if(::bind(file_descriptor_, (sockaddr*)&socket_address, sizeof(sockaddr_un)) == -1)
        {
            throw std::runtime_error("Failed to bind socket to address");
        }

        if(::listen(file_descriptor_, 5) == -1)
        {
            throw std::runtime_error("Failed to listen on socket");
        }
    }

    ~unix_domain_socket()
    {
        if(file_descriptor_ == -1) return;
        ::close(file_descriptor_);
        ::unlink(path_.c_str());
    }

    int file_descriptor_;

    std::filesystem::path path_;
};

TEST(UnixDomainSocketIoStream, DefaultConstructOpen)
{
    const auto socket_path = R"(./snail-test-socket-1)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream;

    EXPECT_FALSE(stream.is_open());

    stream.open(socket_path);

    EXPECT_TRUE(stream.is_open());
}

TEST(UnixDomainSocketIoStream, ConstructOpen)
{
    const auto socket_path = R"(./snail-test-socket-2)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());
}

TEST(UnixDomainSocketIoStream, ConstructOpenInvalid)
{
    const auto                  socket_path = R"(./snail-non-existing-socket)";
    unix_domain_socket_iostream stream(socket_path);
    EXPECT_FALSE(stream.is_open());
}

TEST(UnixDomainSocketIoStream, ReadWrite)
{
    using namespace std::literals;

    const auto socket_path = R"(./snail-test-socket-3)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());

    const auto test_data_in = "in-data\n"sv;

    const auto client_file_descriptor_ = ::accept(socket.file_descriptor_, nullptr, nullptr);
    ASSERT_NE(client_file_descriptor_, -1);

    const auto written_bytes = ::write(client_file_descriptor_, test_data_in.data(), static_cast<int>(test_data_in.size()));
    EXPECT_EQ(written_bytes, test_data_in.size());

    std::string buffer;
    buffer.resize(test_data_in.size());
    stream.read(buffer.data(), test_data_in.size());
    EXPECT_EQ(buffer, test_data_in);

    const auto test_data_out = "out-data\n"sv;

    stream.write(test_data_out.data(), test_data_out.size());
    stream.flush();

    buffer.clear();
    buffer.resize(test_data_out.size() + 1);
    buffer.back()         = '\0';
    const auto read_bytes = ::read(client_file_descriptor_, buffer.data(), static_cast<int>(test_data_out.size()));
    EXPECT_EQ(read_bytes, test_data_out.size());

    ::close(client_file_descriptor_);
    ::unlink(socket_path);
}
