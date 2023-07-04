
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace snail::perf_data {

struct build_id
{
    static constexpr std::size_t max_size = 20;

    std::size_t                     size_;
    std::array<std::byte, max_size> buffer_;

    std::string to_string() const;

    [[nodiscard]] friend bool operator==(const build_id& lhs, const build_id& rhs)
    {
        return lhs.size_ == rhs.size_ &&
               std::equal(lhs.buffer_.begin(), lhs.buffer_.begin() + lhs.size_, rhs.buffer_.begin());
    }
};

} // namespace snail::perf_data
