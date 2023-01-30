#pragma once

#include <filesystem>
#include <iostream>

namespace snail::jsonrpc {

class pipe_iostream : public std::iostream
{
public:
    pipe_iostream();
    explicit pipe_iostream(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    pipe_iostream(pipe_iostream&& other);
    pipe_iostream(const pipe_iostream& other) = delete;

    ~pipe_iostream();

    pipe_iostream& operator=(pipe_iostream&& other);
    pipe_iostream& operator=(const pipe_iostream& other) = delete;

    void open(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    [[nodiscard]] bool is_open() const;

    void close();
};

} // namespace snail::jsonrpc
