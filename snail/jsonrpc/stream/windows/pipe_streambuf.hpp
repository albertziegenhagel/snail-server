#pragma once

#include <snail/jsonrpc/stream/generic/streambuf_base.hpp>

namespace snail::jsonrpc {

class pipe_streambuf : public streambuf_base
{
public:
    pipe_streambuf();
    explicit pipe_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode);

    pipe_streambuf(pipe_streambuf&& other)      = delete;
    pipe_streambuf(const pipe_streambuf& other) = delete;

    ~pipe_streambuf();

    pipe_streambuf& operator=(pipe_streambuf&& other)      = delete;
    pipe_streambuf& operator=(const pipe_streambuf& other) = delete;

    virtual void open(const std::filesystem::path& path, std::ios_base::openmode mode) override;

    [[nodiscard]] virtual bool is_open() const override;

    virtual void close() override;

private:
    struct impl;
    std::unique_ptr<impl> impl_;

    virtual std::streamsize read(char_type* buffer, std::streamsize bytes_to_read) override;
    virtual std::streamsize write(const char_type* buffer, std::streamsize bytes_to_write) override;
};

} // namespace snail::jsonrpc
