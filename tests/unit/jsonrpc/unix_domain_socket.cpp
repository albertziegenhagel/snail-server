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
    const auto socket_path = R"(./snail-test-socket-def-con-open)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream;

    EXPECT_FALSE(stream.is_open());
    EXPECT_TRUE(stream.good());

    stream.open(socket_path);

    EXPECT_TRUE(stream.is_open());
    EXPECT_TRUE(stream.good());
}

TEST(UnixDomainSocketIoStream, ConstructOpen)
{
    const auto socket_path = R"(./snail-test-socket-con-open)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());
    EXPECT_TRUE(stream.good());
}

TEST(PipeIoStream, ConstructOpenAndClose)
{
    const auto socket_path = R"(./snail-test-socket-con-open)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());
    EXPECT_TRUE(stream.good());

    stream.close();
    EXPECT_FALSE(stream.is_open());
}

TEST(UnixDomainSocketIoStream, ConstructOpenMove)
{
    const auto socket_path = R"(./snail-test-socket-con-open)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());
    EXPECT_TRUE(stream.good());

    unix_domain_socket_iostream stream_2(std::move(stream));
    EXPECT_FALSE(stream.is_open());
    EXPECT_TRUE(stream_2.is_open());
    EXPECT_TRUE(stream_2.good());
}

TEST(UnixDomainSocketIoStream, ConstructOpenMoveAssign)
{
    const auto socket_path = R"(./snail-test-socket-con-open-move-assign)";
    const auto socket      = unix_domain_socket(socket_path);

    unix_domain_socket_iostream stream(socket_path);
    EXPECT_TRUE(stream.is_open());
    EXPECT_TRUE(stream.good());

    unix_domain_socket_iostream stream_2;
    EXPECT_FALSE(stream_2.is_open());
    EXPECT_TRUE(stream_2.good());

    stream_2 = std::move(stream);
    EXPECT_FALSE(stream.is_open());
    EXPECT_TRUE(stream_2.is_open());
    EXPECT_TRUE(stream_2.good());
}

TEST(UnixDomainSocketIoStream, ConstructOpenMoveAssignOpen)
{
    const auto socket_path_1 = R"(./snail-test-socket-con-open-move-assign-open-1)";
    const auto socket_path_2 = R"(./snail-test-socket-con-open-move-assign-open-2)";
    const auto socket_1      = unix_domain_socket(socket_path_1);
    const auto socket_2      = unix_domain_socket(socket_path_2);

    unix_domain_socket_iostream stream_1(socket_path_1);
    EXPECT_TRUE(stream_1.is_open());
    EXPECT_TRUE(stream_1.good());

    unix_domain_socket_iostream stream_2(socket_path_2);
    EXPECT_TRUE(stream_2.is_open());
    EXPECT_TRUE(stream_2.good());

    stream_2 = std::move(stream_1);
    EXPECT_FALSE(stream_1.is_open());
    EXPECT_TRUE(stream_2.is_open());
    EXPECT_TRUE(stream_2.good());
}

TEST(UnixDomainSocketIoStream, ConstructOpenInvalid)
{
    const auto                  socket_path = R"(./snail-non-existing-socket)";
    unix_domain_socket_iostream stream(socket_path);
    EXPECT_FALSE(stream.is_open());
    EXPECT_FALSE(stream.good());
}

TEST(UnixDomainSocketIoStream, OpenInvalid)
{
    const auto                  socket_path = R"(./snail-non-existing-socket)";
    unix_domain_socket_iostream stream;
    stream.open(socket_path);
    EXPECT_FALSE(stream.is_open());
    EXPECT_FALSE(stream.good());
}

TEST(UnixDomainSocketIoStream, ReadNonOpen)
{
    unix_domain_socket_iostream stream;
    EXPECT_FALSE(stream.is_open());
    EXPECT_TRUE(stream.good());

    char c = '\0';
    stream.read(&c, 1);
    EXPECT_FALSE(stream.good());
}

TEST(UnixDomainSocketIoStream, WriteNonOpen)
{
    unix_domain_socket_iostream stream;
    EXPECT_FALSE(stream.is_open());
    EXPECT_TRUE(stream.good());

    char c = '\0';
    stream.write(&c, 1);
    EXPECT_FALSE(stream.good());
}

TEST(UnixDomainSocketIoStream, ReadWrite)
{
    using namespace std::literals;

    const auto socket_path = R"(./snail-test-socket-read-write)";
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
    stream.read(buffer.data(), static_cast<std::streamsize>(test_data_in.size()));
    EXPECT_EQ(buffer, test_data_in);

    const auto test_data_out = "out-data\n"sv;

    stream.write(test_data_out.data(), static_cast<std::streamsize>(test_data_out.size()));
    stream.flush();

    buffer.clear();
    buffer.resize(test_data_out.size() + 1);
    buffer.back()         = '\0';
    const auto read_bytes = ::read(client_file_descriptor_, buffer.data(), static_cast<int>(test_data_out.size()));
    EXPECT_EQ(read_bytes, test_data_out.size());

    ::close(client_file_descriptor_);
    ::unlink(socket_path);
}
