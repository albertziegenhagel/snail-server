
#pragma once

#include <filesystem>
#include <fstream>
#include <span>
#include <bit>

namespace snail::perf_data {

namespace parser {

struct event_header_view;
struct event_attributes;

} // namespace parser

namespace detail {

struct perf_data_file_header_data;

} // namespace detail

class event_observer;

class perf_data_file
{
public:
    perf_data_file() = default;
    explicit perf_data_file(const std::filesystem::path& file_path);

    ~perf_data_file();

    void open(const std::filesystem::path& file_path);

    void close();
    
    void process(event_observer& callbacks);
private:
    std::ifstream file_stream_;

    std::unique_ptr<detail::perf_data_file_header_data> header_;
};

class event_observer
{
public:
    virtual ~event_observer() = default;

    virtual void handle(const parser::event_header_view& /*event_header*/, const parser::event_attributes& /*attributes*/, std::span<const std::byte> /*event_data*/, std::endian /*byte_order*/) {}
};

} // namespace snail::perf_data
