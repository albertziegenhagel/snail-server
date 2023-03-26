#pragma once

#include <filesystem>
#include <ios>
#include <memory>
#include <streambuf>

namespace snail::jsonrpc {

class streambuf_base : public std::streambuf
{
public:
    streambuf_base() = default;

    streambuf_base(streambuf_base&& other) noexcept;
    streambuf_base(const streambuf_base& other) = delete;

    streambuf_base& operator=(streambuf_base&& other) noexcept;
    streambuf_base& operator=(const streambuf_base& other) = delete;

    virtual void open(const std::filesystem::path& path, std::ios_base::openmode mode) = 0;

    [[nodiscard]] virtual bool is_open() const = 0;

    virtual void close() = 0;

protected:
    virtual int sync() override;

    // virtual std::streamsize xsgetn(char_type* s, std::streamsize n) override;
    virtual int_type underflow() override;

    // virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override;
    virtual int_type overflow(int_type c = traits_type::eof()) override;

    virtual std::streamsize read(char_type* buffer, std::streamsize bytes_to_read)         = 0;
    virtual std::streamsize write(const char_type* buffer, std::streamsize bytes_to_write) = 0;

private:
    std::ios_base::openmode mode_;

    std::streamsize get_area_size_;
    std::streamsize put_area_size_;

    std::unique_ptr<char[]> buffer_;

    void init_buffer();
};

} // namespace snail::jsonrpc
