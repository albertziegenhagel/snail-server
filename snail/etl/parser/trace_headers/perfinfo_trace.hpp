
#pragma once

#include <cstdint>

#include <type_traits>

#include <snail/etl/parser/extract.hpp>
#include <snail/etl/parser/trace.hpp>

namespace snail::etl::parser {

// See _PERFINFO_TRACE_HEADER in ntwmi.h
struct perfinfo_trace_header_view : private extract_view_base
{
    using extract_view_base::buffer;
    using extract_view_base::extract_view_base;

    static constexpr inline std::uint16_t pebs_index_flag   = 0b1000'0000'0000'0000; // `TRACE_HEADER_PEBS_INDEX_FLAG` from ntwmi.h
    static constexpr inline std::uint16_t pmc_counters_mask = 0b0000'0111'0000'0000; // `TRACE_HEADER_PMC_COUNTERS_MASK` from ntwmi.h
    static constexpr inline std::uint16_t ext_items_mask    = pebs_index_flag | pmc_counters_mask;

    static std::size_t peak_extended_size(std::span<const std::byte> buffer)
    {
        const auto ext_header = common::parser::extract<std::uint16_t>(buffer, 0, etl_file_byte_order);
        const auto has_pebs   = (ext_header & pebs_index_flag) != 0;
        const auto pmc_count  = static_cast<std::uint8_t>((ext_header & pmc_counters_mask) >> 8);
        return (has_pebs ? 8 : 0) + pmc_count * 8;
    }

    inline auto version() const { return static_cast<std::uint16_t>(extract<std::uint16_t>(0) & ~ext_items_mask); }
    inline auto header_type() const { return extract<trace_header_type>(2); }
    inline auto header_flags() const { return extract<std::uint8_t>(3); }

    inline auto packet() const { return wmi_trace_packet_view(buffer().subspan(4)); }

    inline auto system_time() const { return extract<std::uint64_t>(4 + wmi_trace_packet_view::static_size); }

    static inline constexpr std::size_t static_size = 12 + wmi_trace_packet_view::static_size; // without extended data!

    inline bool has_ext_pebs() const { return (extract<std::uint16_t>(0) & pebs_index_flag) != 0; }
    inline auto ext_pmc_count() const { return static_cast<std::uint8_t>((extract<std::uint16_t>(0) & pmc_counters_mask) >> 8); }

    inline auto ext_pebs() const
    {
        assert(has_ext_pebs());
        return extract<std::uint64_t>(static_size);
    }

    inline auto ext_pmc(std::size_t index) const
    {
        assert(index < ext_pmc_count());
        return extract<std::uint64_t>(static_size + (has_ext_pebs() ? 8 : 0) + index * 8);
    }
};

} // namespace snail::etl::parser
