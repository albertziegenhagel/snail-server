
#pragma once

#include <iostream>
#include <bit>
#include <span>

namespace perfreader::utility {

class memory_istream_buffer : public std::streambuf
{
public:
    explicit memory_istream_buffer(std::span<std::byte> buffer);

    virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;

    virtual pos_type seekpos(pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;
private:
    std::span<std::byte>& buffer_;
};

class memory_istream : public std::istream
{
public:
    memory_istream(std::span<std::byte> buffer) :
        std::istream(new memory_istream_buffer(buffer))
    {}

    ~memory_istream() noexcept override { delete rdbuf(); }

    memory_istream(const memory_istream& other) = delete;
    memory_istream(memory_istream&& other) noexcept = delete;

    memory_istream& operator=(const memory_istream& other) = delete;
    memory_istream& operator=(memory_istream&& other) noexcept = delete;
};

} // namespace perfreader::utility
