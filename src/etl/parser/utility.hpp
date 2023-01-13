
#pragma once

#include <cstdint>
#include <cstddef>

#include "etl/guid.hpp"

#include "etl/parser/extract.hpp"

namespace snail::etl::parser {

// See SYSTEMTIME from minwinbase.h
struct system_time_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto year() const { return extract<std::int16_t>(0); }
    inline auto month() const { return extract<std::int16_t>(2); }
    inline auto day_of_week() const { return extract<std::int16_t>(4); }
    inline auto day() const { return extract<std::int16_t>(6); }
    inline auto hour() const { return extract<std::int16_t>(8); }
    inline auto minute() const { return extract<std::int16_t>(10); }
    inline auto second() const { return extract<std::int16_t>(12); }
    inline auto milliseconds() const { return extract<std::int16_t>(14); }

    static inline constexpr std::size_t static_size = 16;
};

// See TIME_ZONE_INFORMATION from timezoneapi.h
struct time_zone_information_view : private extract_view_base
{
    static_assert(sizeof(char16_t) == sizeof(std::uint16_t));

    using extract_view_base::extract_view_base;
    
    inline auto bias() const { return extract<std::uint32_t>(0); }

    inline auto standard_name() const { return extract_u16string(4, standard_name_length); }
    inline auto standard_date() const { return system_time_view(buffer().subspan(68)); }
    inline auto standard_bias() const { return extract<std::int32_t>(68 + system_time_view::static_size); }

    inline auto daylight_name() const { return extract_u16string(72 + system_time_view::static_size, daylight_name_length); }
    inline auto daylight_date() const { return system_time_view(buffer().subspan(136 + system_time_view::static_size)); }
    inline auto daylight_bias() const { return extract<std::int32_t>(136 + system_time_view::static_size*2); }

    static inline constexpr std::size_t static_size = 140 + system_time_view::static_size*2;

private:
    mutable std::optional<std::size_t> standard_name_length;
    mutable std::optional<std::size_t> daylight_name_length;
};

// See GUID from guiddef.h
struct guid_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto data_1() const { return extract<std::uint32_t>(0); }
    inline auto data_2() const { return extract<std::uint16_t>(4); }
    inline auto data_3() const { return extract<std::uint16_t>(6); }
    inline auto data_4() const { return std::span<const std::uint8_t, 8>(reinterpret_cast<const std::uint8_t*>(buffer().data() + 8), 8); }

    static inline constexpr std::size_t static_size = 16;

    etl::guid instantiate() const
    {
        const auto data_4_ = data_4();
        return etl::guid{data_1(), data_2(), data_3(), {data_4_[0], data_4_[1], data_4_[2], data_4_[3], data_4_[4], data_4_[5], data_4_[6], data_4_[7]}};
    }
};

// See https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/f992ad60-0fe4-4b87-9fed-beb478836861
struct sid_view : private extract_view_base
{
    using extract_view_base::extract_view_base;
    
    inline auto revision() const { return extract<std::uint8_t>(0); }
    inline auto sub_authority_count() const { return extract<std::uint8_t>(1); }
    inline auto identifier_authority() const { return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(buffer().data() + 2), 6); }
    inline auto sub_authority() const
    {
        assert(sub_authority_count() <= 16); // 16 is the max buffer length
        return std::span<const std::uint32_t>(reinterpret_cast<const std::uint32_t*>(buffer().data() + 8), sub_authority_count());
    }

    std::size_t dynamic_size() const
    {
        return 8 + sub_authority_count()*4;
    }
};

} // namespace snail::etl::parser
