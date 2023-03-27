
#include <snail/perf_data/perf_data_file.hpp>

#include <array>
#include <bit>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <snail/common/bit_flags.hpp>
#include <snail/common/cast.hpp>
#include <snail/common/chunked_reader.hpp>
#include <snail/common/stream_position.hpp>

#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/parser/event_attributes.hpp>
#include <snail/perf_data/parser/header.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

#include <snail/perf_data/detail/attributes_database.hpp>
#include <snail/perf_data/detail/file_header.hpp>
#include <snail/perf_data/detail/metadata.hpp>

#include <snail/common/detail/dump.hpp>

using namespace snail;
using namespace snail::perf_data;

namespace {

inline constexpr std::size_t max_chunk_size = 0xFFFF + 1;

template<typename T>
    requires std::is_integral_v<T>
T read_int(std::ifstream& file_stream, std::endian data_byte_order)
{
    T value;
    file_stream.read(reinterpret_cast<char*>(&value), sizeof(T));

    if constexpr(sizeof(T) > 1)
    {
        if(data_byte_order != std::endian::native)
        {
            return std::byteswap(value);
        }

        return value;
    }
    else
    {
        return value;
    }
}

std::string read_string(std::ifstream& file_stream, std::endian data_byte_order)
{
    const auto length = read_int<std::uint32_t>(file_stream, data_byte_order);

    std::vector<char> buffer;
    buffer.resize(length + 1);
#if defined(__GNUC__) && !defined(__clang__) // workaround for invalid GCC diagnostic
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    buffer.back() = '\0'; // just to make sure we definitely have a valid string termination
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif

    file_stream.read(buffer.data(), length);

    return {buffer.data()};
}

std::vector<std::string> read_string_list(std::ifstream& file_stream, std::endian data_byte_order)
{
    const auto num_strings = read_int<std::uint32_t>(file_stream, data_byte_order);

    std::vector<std::string> result;
    result.reserve(num_strings);
    for(std::size_t i = 0; i < num_strings; ++i)
    {
        result.push_back(read_string(file_stream, data_byte_order));
    }
    return result;
}

void read_attributes_section(std::ifstream&                            file_stream,
                             const detail::perf_data_file_header_data& header,
                             detail::event_attributes_database&        attributes_database)
{
    constexpr auto attribute_static_size = parser::event_attributes_view::static_size + parser::file_section_view::static_size;
    assert(attribute_static_size <= header.attribute_size);

    struct attribute_and_ids
    {
        parser::event_attributes   attributes;
        std::vector<std::uint64_t> ids;
    };
    std::vector<attribute_and_ids> attributes;

    auto reader = common::chunked_reader<max_chunk_size>(file_stream, header.attributes.offset, header.attributes.size);
    while(reader.keep_going())
    {
        const auto buffer = reader.retrieve_data(attribute_static_size);
        if(buffer.size() != attribute_static_size) continue;

        const auto attribute_view = parser::event_attributes_view(buffer, header.byte_order);

        if(attribute_view.size() != parser::event_attributes_view::static_size)
        {
            std::cout << std::format(
                             "Invalid event in perf.data: Event attribute size is given as {} bytes but should be {}.",
                             attribute_view.size(),
                             parser::event_attributes_view::static_size)
                      << std::endl;
            return;
        }

        const auto ids_section = parser::file_section_view(buffer.subspan(parser::event_attributes_view::static_size), header.byte_order);

        std::vector<std::uint64_t> ids;
        ids.reserve(ids_section.size() / sizeof(std::uint64_t));

        {
            const auto resetter = common::stream_position_resetter(file_stream);

            auto id_reader = common::chunked_reader<0xFF>(file_stream, ids_section.offset(), ids_section.size());
            while(id_reader.keep_going())
            {
                const auto id_buffer = id_reader.retrieve_data(sizeof(std::uint64_t));
                if(id_buffer.size() != sizeof(std::uint64_t)) continue;
                ids.push_back(common::parser::extract<std::uint64_t>(id_buffer, 0, header.byte_order));
            }
        }

        attributes.push_back(attribute_and_ids{
            .attributes = attribute_view.instantiate(),
            .ids        = std::move(ids)});
    }

    attributes_database.all_attributes.clear();
    attributes_database.id_to_attributes.clear();
    attributes_database.all_attributes.reserve(attributes.size());
    for(const auto& data : attributes)
    {
        attributes_database.all_attributes.push_back(data.attributes);
        for(const auto id : data.ids)
        {
            attributes_database.id_to_attributes[id] = &attributes_database.all_attributes.back();
        }
    }
    attributes_database.validate();
}

void read_event_types_section(std::ifstream& file_stream, const detail::perf_data_file_header_data& header)
{
    (void)file_stream;
    (void)header;
    // constexpr auto attribute_static_size = parser::event_attributes_view::static_size + parser::file_section_view::static_size;
    // assert(attribute_static_size <= header.attribute_size);

    // auto reader = common::chunked_reader<max_chunk_size>(file_stream, header.event_types.offset, header.event_types.size);
    // while(reader.keep_going())
    // {
    //     const auto buffer = reader.retrieve_data(attribute_static_size);
    //     if(buffer.size() != attribute_static_size) continue;
    // }
}

parser::header_feature proceed_to_next_feature(const parser::header_feature_flags& feature_flags, parser::header_feature current_feature)
{
    assert(static_cast<std::size_t>(current_feature) < static_cast<std::size_t>(parser::header_feature::last_feature));

    auto next_feature_index = static_cast<std::size_t>(current_feature) + 1;
    while(!feature_flags.test(parser::header_feature(next_feature_index)) && next_feature_index < static_cast<std::size_t>(parser::header_feature::last_feature))
    {
        ++next_feature_index;
    }
    return parser::header_feature(next_feature_index);
}

void read_metadata(std::ifstream&                            file_stream,
                   const detail::perf_data_file_header_data& header,
                   detail::perf_data_metadata&               metadata)
{
    const auto active_features_count = header.additional_features.count();
    if(active_features_count == 0) return;

    const auto metadata_headers_size = active_features_count * parser::file_section_view::static_size;
    auto       reader                = common::chunked_reader<512>(file_stream, header.data.offset + header.data.size, metadata_headers_size);

    auto current_feature = parser::header_feature(0);
    while(reader.keep_going())
    {
        current_feature = proceed_to_next_feature(header.additional_features, current_feature);

        const auto metadata_section_buffer = reader.retrieve_data(parser::file_section_view::static_size);
        if(metadata_section_buffer.size() != parser::file_section_view::static_size) continue;

        const auto metadata_section = parser::file_section_view(metadata_section_buffer, header.byte_order);

        {
            const auto resetter = common::stream_position_resetter(file_stream);

            file_stream.seekg(common::narrow_cast<std::streamoff>(metadata_section.offset()));

            switch(current_feature)
            {
            // case parser::header_feature::tracing_data:
            // case parser::header_feature::build_id:
            case parser::header_feature::hostname:
                metadata.hostname = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::osrelease:
                metadata.os_release = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::version:
                metadata.version = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::arch:
                metadata.arch = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::nr_cpus:
                metadata.nr_cpus = {
                    .nr_cpus_available = read_int<std::uint32_t>(file_stream, header.byte_order),
                    .nr_cpus_online    = read_int<std::uint32_t>(file_stream, header.byte_order)};
                break;
            case parser::header_feature::cpu_desc:
                metadata.cpu_desc = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::cpu_id:
                metadata.cpu_id = read_string(file_stream, header.byte_order);
                break;
            case parser::header_feature::total_mem:
                metadata.total_mem = read_int<std::uint64_t>(file_stream, header.byte_order);
                break;
            case parser::header_feature::cmdline:
                metadata.cmdline = read_string_list(file_stream, header.byte_order);
                break;
            case parser::header_feature::event_desc:
            {
                const auto                  nr_events = read_int<std::uint32_t>(file_stream, header.byte_order);
                [[maybe_unused]] const auto attr_size = read_int<std::uint32_t>(file_stream, header.byte_order);
                assert(attr_size == parser::event_attributes_view::static_size);
                std::array<std::byte, parser::event_attributes_view::static_size> attribute_buffer;
                for(std::uint32_t event_i = 0; event_i < nr_events; ++event_i)
                {
                    file_stream.read(reinterpret_cast<char*>(attribute_buffer.data()), attribute_buffer.size());
                    const auto                 attribute_view = parser::event_attributes_view(attribute_buffer, header.byte_order);
                    const auto                 nr_ids         = read_int<std::uint32_t>(file_stream, header.byte_order);
                    auto                       event_string   = read_string(file_stream, header.byte_order);
                    std::vector<std::uint64_t> ids;
                    for(std::uint32_t id_i = 0; id_i < nr_ids; ++id_i)
                    {
                        ids.push_back(read_int<std::uint64_t>(file_stream, header.byte_order));
                    }
                    metadata.event_desc.push_back(detail::perf_data_metadata::event_desc_data{
                        .attribute    = attribute_view.instantiate(),
                        .event_string = std::move(event_string),
                        .ids          = std::move(ids)});
                }
                break;
            }
            // case parser::header_feature::cpu_topology:
            // case parser::header_feature::numa_topology:
            // case parser::header_feature::branch_stack:
            // case parser::header_feature::pmu_mappings:
            // case parser::header_feature::group_desc:
            // case parser::header_feature::aux_trace:
            // case parser::header_feature::stat:
            // case parser::header_feature::cache:
            case parser::header_feature::sample_time:
                metadata.sample_time = {
                    .start = std::chrono::nanoseconds(read_int<std::uint64_t>(file_stream, header.byte_order)),
                    .end   = std::chrono::nanoseconds(read_int<std::uint64_t>(file_stream, header.byte_order))};
                break;
            // case parser::header_feature::mem_topology:
            case parser::header_feature::clockid:
                metadata.clockid = read_int<std::uint64_t>(file_stream, header.byte_order);
                break;
            // case parser::header_feature::dir_format:
            // case parser::header_feature::bpf_prog_info:
            // case parser::header_feature::bpf_btf:
            // case parser::header_feature::compressed:
            // case parser::header_feature::cpu_pmu_caps:
            // case parser::header_feature::clock_data:
            // case parser::header_feature::hybrid_topology:
            // case parser::header_feature::pmu_caps:
            //     break;
            default:
                break;
            }
        }
    }
}

void read_data_section(std::ifstream&                            file_stream,
                       const detail::perf_data_file_header_data& header,
                       const detail::event_attributes_database&  attributes_database,
                       event_observer&                           callbacks)
{
    auto reader = common::chunked_reader<max_chunk_size>(file_stream, header.data.offset, header.data.size);
    while(reader.keep_going())
    {
        const auto event_header_buffer = reader.retrieve_data(parser::event_header_view::static_size, true);
        if(event_header_buffer.size() != parser::event_header_view::static_size) continue;

        const auto event_header = parser::event_header_view(event_header_buffer, header.byte_order);

        if(event_header.size() < parser::event_header_view::static_size)
        {
            std::cout << std::format(
                             "Invalid event in perf.data: Event size is given as {} bytes but header is at least {}.",
                             event_header.size(),
                             parser::event_header_view::static_size)
                      << std::endl;
            return;
        }

        const auto full_event_buffer = reader.retrieve_data(event_header.size());
        if(full_event_buffer.size() != event_header.size()) continue;

        const auto event_buffer = full_event_buffer.subspan(parser::event_header_view::static_size);

        const auto& event_attributes = attributes_database.get_event_attributes(header, event_header, event_buffer);

        callbacks.handle(event_header, event_attributes, event_buffer, header.byte_order);
    }
}

} // namespace

perf_data_file::perf_data_file(const std::filesystem::path& file_path)
{
    open(file_path);
}

void perf_data_file::open(const std::filesystem::path& file_path)
{
    file_stream_.open(file_path, std::ios_base::binary);

    if(!file_stream_.is_open())
    {
        throw std::runtime_error(std::format("Could not open file {}", file_path.string()));
    }

    std::array<std::byte, parser::header_view::static_size> file_buffer_data;
    file_stream_.read(reinterpret_cast<char*>(file_buffer_data.data()), file_buffer_data.size());

    const auto read_bytes = file_stream_.tellg() - std::streampos(0);

    if(read_bytes < static_cast<std::streamoff>(parser::header_view::static_size))
    {
        close();
        throw std::runtime_error(std::format(
            "Invalid perf.data file: insufficient size for header. Expected {} but read only {}.",
            parser::header_view::static_size,
            read_bytes));
    }

    const auto file_buffer = std::span(file_buffer_data);

    // This is actually the string "PERFILE2" encoded as 64-bit integer
    // and it's corresponding byte swapped value.
    constexpr std::uint64_t magic_v2         = 0x32454c4946524550ULL;
    constexpr std::uint64_t magic_v2_swapped = 0x50455246494c4532ULL;

    const auto magic = common::parser::extract<std::uint64_t>(file_buffer, 0, std::endian::native);

    if(magic != magic_v2 &&
       magic != magic_v2_swapped)
    {
        const auto magic_str = std::string_view(reinterpret_cast<const char*>(file_buffer.data()), 8);
        close();
        throw std::runtime_error(std::format(
            "Invalid perf.data file: Invalid magic header.\n  Expected '{}' (little endian) or '{}' (big endian) but got '{}'.",
            "PERFILE2",
            "2ELIFREP",
            magic_str));
    }

    constexpr auto non_native_byte_order = std::endian::native == std::endian::little ?
                                               std::endian::big :
                                               std::endian::little;

    const auto file_byte_order = magic == magic_v2 ?
                                     std::endian::native :
                                     non_native_byte_order;

    const auto header = parser::header_view(file_buffer, file_byte_order);
    assert(header.magic() == magic_v2);

    if(header.size() != parser::header_view::static_size)
    {
        close();
        throw std::runtime_error(std::format(
            "Invalid perf.data file: Invalid header size.\n  Expected {} but got {}.",
            parser::header_view::static_size,
            header.size()));
    }

    header_ = std::make_unique<detail::perf_data_file_header_data>(detail::perf_data_file_header_data{
        .byte_order     = file_byte_order,
        .size           = header.size(),
        .attribute_size = header.attributes_size(),
        .attributes     = detail::perf_data_file_header_data::section_data{
                                                                           .offset = header.attributes().offset(),
                                                                           .size   = header.attributes().size(),
                                                                           },
        .data = detail::perf_data_file_header_data::section_data{
                                                                           .offset = header.data().offset(),
                                                                           .size   = header.data().size(),
                                                                           },
        .event_types = detail::perf_data_file_header_data::section_data{
                                                                           .offset = header.event_types().offset(),
                                                                           .size   = header.event_types().size(),
                                                                           },
        .additional_features = header.additional_features()
    });
}

perf_data_file::~perf_data_file() = default;

void perf_data_file::close()
{
    file_stream_.close();
    header_ = nullptr;
}

void perf_data_file::process(event_observer& callbacks)
{
    if(!file_stream_.is_open())
    {
        throw std::runtime_error("Cannot process file: file is not open.");
    }

    if(header_ == nullptr)
    {
        throw std::runtime_error("Cannot process file: missing header data.");
    }

    detail::event_attributes_database attributes_database;

    if(!header_->additional_features.test(parser::header_feature::event_desc))
    {
        read_attributes_section(file_stream_, *header_, attributes_database);
    }

    metadata_ = std::make_unique<detail::perf_data_metadata>();
    read_metadata(file_stream_, *header_, *metadata_);

    if(header_->additional_features.test(parser::header_feature::event_desc))
    {
        metadata_->extract_event_attributes_database(attributes_database);
    }

    read_data_section(file_stream_, *header_, attributes_database, callbacks);

    read_event_types_section(file_stream_, *header_);
}

const detail::perf_data_metadata& perf_data_file::metadata() const
{
    assert(metadata_ != nullptr);
    return *metadata_;
}
