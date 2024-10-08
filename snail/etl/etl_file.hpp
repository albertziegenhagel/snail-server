
#pragma once

#include <bit>
#include <filesystem>
#include <fstream>
#include <span>

#include <snail/common/date_time.hpp>
#include <snail/common/ms_xca_compression_format.hpp>

#include <snail/etl/parser/log_file_mode.hpp>
#include <snail/etl/parser/trace_headers/fwd.hpp>

namespace snail::etl {

namespace parser {

struct wmi_buffer_header_view;

} // namespace parser

class event_observer;

class etl_file
{
public:
    struct header_data
    {
        common::nt_sys_time start_time;
        common::nt_sys_time end_time;

        std::uint64_t start_time_qpc_ticks;
        std::uint64_t qpc_frequency;

        std::uint32_t pointer_size;

        std::uint32_t number_of_processors;

        std::uint32_t number_of_buffers;

        std::uint32_t buffer_size;

        parser::log_file_mode_flags log_file_mode;

        common::ms_xca_compression_format compression_format;
    };

    etl_file() = default;
    explicit etl_file(const std::filesystem::path& file_path);

    void open(const std::filesystem::path& file_path);

    void close();

    void process(event_observer& callbacks);

    const header_data& header() const;

private:
    std::ifstream file_stream_;
    header_data   header_;
};

class event_observer
{
public:
    virtual ~event_observer() = default;

    virtual void handle_buffer(const etl_file::header_data& /*file_header*/, const parser::wmi_buffer_header_view& /*buffer_header*/) {}

    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::system_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::compact_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::perfinfo_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::event_header_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::full_header_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
    virtual void handle(const etl_file::header_data& /*file_header*/, const parser::instance_trace_header_view& /*trace_header*/, std::span<const std::byte> /*user_data*/) {}
};

} // namespace snail::etl
