#pragma once

#include <snail/jsonrpc/stream/generic/streambuf_base.hpp>

namespace snail::jsonrpc {

class unix_domain_socket_streambuf : public streambuf_base
{
public:
    unix_domain_socket_streambuf();
    explicit unix_domain_socket_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode);

    unix_domain_socket_streambuf(unix_domain_socket_streambuf&& other);
    unix_domain_socket_streambuf(const unix_domain_socket_streambuf& other) = delete;

    ~unix_domain_socket_streambuf();

    unix_domain_socket_streambuf& operator=(unix_domain_socket_streambuf&& other);
    unix_domain_socket_streambuf& operator=(const unix_domain_socket_streambuf& other) = delete;

    virtual void open(const std::filesystem::path& path, std::ios_base::openmode mode) override;

    [[nodiscard]] virtual bool is_open() const override;

    virtual void close() override;

private:
    inline static constexpr auto invalid_unix_domain_socket_file_ = -1;

    int unix_domain_socket_file_;

    virtual std::streamsize read(char_type* buffer, std::streamsize bytes_to_read) override;
    virtual std::streamsize write(const char_type* buffer, std::streamsize bytes_to_write) override;
};

} // namespace snail::jsonrpc
