
#pragma once

#include <fstream>
#include <filesystem>
#include <span>
#include <bit>

#include <snail/etl/parser/trace_headers/fwd.hpp>

namespace snail::etl {

class event_observer;

class etl_file
{
public:
    struct header_data
    {
        std::uint32_t pointer_size = 0;
    };

    etl_file() = default;
    explicit etl_file(const std::filesystem::path& file_path);

    void open(const std::filesystem::path& file_path);

    void close();
    
    void process(event_observer& callbacks);
private:
    std::ifstream file_stream_;
    header_data   header_;
};

class event_observer
{
public:
    virtual ~event_observer() = default;

    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::system_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::compact_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::perfinfo_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::event_header_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::full_header_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::instance_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
};

} // namespace snail::etl
