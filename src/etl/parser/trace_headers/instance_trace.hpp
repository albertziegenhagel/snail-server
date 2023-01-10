
#pragma once

#include <cstdint>

#include <type_traits>

#include "binarystream/fwd/binarystream.hpp"

#include "etl/parser/trace.hpp"

#include "etl/parser/utility.hpp"

namespace perfreader::etl::parser {

// See _EVENT_INSTANCE_GUID_HEADER in ntwmi.h
struct instance_trace_header
{
    std::uint16_t size;
    trace_header_type header_type;
    std::uint8_t  header_flags;
    
    std::uint64_t version; // could be `trace_class` as well

    std::uint32_t thread_id;
    std::uint32_t process_id;

    std::uint64_t timestamp;

    parser::guid guid;

    std::uint32_t kernel_time; // kernel_time + user_time could be `timestamp` as well
    std::uint32_t user_time;
    
    std::uint32_t instance_id;
    std::uint32_t parent_instance_id;
    
    parser::guid parent_guid;
};

std::size_t parse(utility::binary_stream_parser& stream, instance_trace_header& data);

} // namespace perfreader::etl::parser
