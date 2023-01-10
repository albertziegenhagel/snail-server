
#pragma once

#include <fstream>
#include <filesystem>
#include <span>
#include <bit>

#include "etl/parser/trace_headers/fwd.hpp"

namespace perfreader::etl {

class event_observer
{
public:
    virtual ~event_observer() = default;

    virtual void handle(const parser::system_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
    virtual void handle(const parser::compact_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
    virtual void handle(const parser::perfinfo_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
    virtual void handle(const parser::event_header_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
    virtual void handle(const parser::full_header_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
    virtual void handle(const parser::instance_trace_header& /*header*/, std::span<std::byte> /*user_data*/) {}
};

class etl_file
{
public:
    etl_file() = default;
    explicit etl_file(const std::filesystem::path& file_path);

    void open(const std::filesystem::path& file_path);

    void process(event_observer& callbacks);

    void close();
private:
    std::ifstream file_stream_;
};

} // namespace perfreader::etl
