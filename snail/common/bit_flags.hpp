
#pragma once

#include <cassert>

#include <bitset>

namespace snail::common {

template<typename FlagsEnum, std::size_t MaxBits>
class bit_flags
{
private:
    using FlagsIntType = std::underlying_type_t<FlagsEnum>;

public:
    inline bit_flags() = default;
    inline explicit bit_flags(unsigned long long data) :
        data_(data)
    {}

    [[nodiscard]] inline bool test(FlagsEnum flag) const
    {
        assert(static_cast<FlagsIntType>(flag) >= 0);
        assert(static_cast<FlagsIntType>(flag) < static_cast<FlagsIntType>(MaxBits));

        return data_.test(static_cast<std::size_t>(flag));
    }

    inline void set(FlagsEnum flag, bool value = true)
    {
        assert(static_cast<FlagsIntType>(flag) >= 0);
        assert(static_cast<FlagsIntType>(flag) < static_cast<FlagsIntType>(MaxBits));

        data_.set(static_cast<std::size_t>(flag), value);
    }

    inline void set() noexcept
    {
        data_.set();
    }
    inline void reset() noexcept
    {
        data_.reset();
    }

    [[nodiscard]] inline std::size_t count() const noexcept
    {
        return data_.count();
    }

    [[nodiscard]] inline std::bitset<MaxBits>& data() noexcept
    {
        return data_;
    }
    [[nodiscard]] inline const std::bitset<MaxBits>& data() const noexcept
    {
        return data_;
    }

private:
    std::bitset<MaxBits> data_;
};

} // namespace snail::common
