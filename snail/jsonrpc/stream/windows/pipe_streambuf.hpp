#pragma once

#include <filesystem>
#include <ios>
#include <memory>
#include <streambuf>

namespace snail::jsonrpc {

class pipe_streambuf : public std::streambuf
{
public:
    pipe_streambuf();
    explicit pipe_streambuf(const std::filesystem::path& path, std::ios_base::openmode mode);

    pipe_streambuf(pipe_streambuf&& other);
    pipe_streambuf(const pipe_streambuf& other) = delete;

    ~pipe_streambuf();

    pipe_streambuf& operator=(pipe_streambuf&& other);
    pipe_streambuf& operator=(const pipe_streambuf& other) = delete;

    void open(const std::filesystem::path& path, std::ios_base::openmode mode);

    [[nodiscard]] bool is_open() const;

    void close();

protected:
    virtual int sync() override;

    // virtual std::streamsize xsgetn(char_type* s, std::streamsize n) override;
    virtual int_type underflow() override;

    // virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override;
    virtual int_type overflow(int_type c = traits_type::eof()) override;

private:
    std::ios_base::openmode mode_;

    void* pipe_handle_;

    std::streamsize get_area_size_;
    std::streamsize put_area_size_;

    std::unique_ptr<char[]> buffer_;

    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);

    void init_buffer();
};

} // namespace snail::jsonrpc
