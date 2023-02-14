#pragma once

#include <filesystem>
#include <iostream>

namespace snail::jsonrpc {

template<typename StreamBufType>
class generic_iostream : public std::iostream
{
public:
    generic_iostream();
    explicit generic_iostream(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    generic_iostream(generic_iostream&& other);
    generic_iostream(const generic_iostream& other) = delete;

    ~generic_iostream();

    generic_iostream& operator=(generic_iostream&& other);
    generic_iostream& operator=(const generic_iostream& other) = delete;

    void open(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    [[nodiscard]] bool is_open() const;

    void close();
};

} // namespace snail::jsonrpc
