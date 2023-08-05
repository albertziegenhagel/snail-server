#pragma once

#include <functional>

#include <snail/common/hash_combine.hpp>

namespace snail::analysis::detail {

template<typename IdType, typename TimeType>
struct id_at
{
    IdType   id;
    TimeType time;

    [[nodiscard]] auto operator<=>(const id_at<IdType, TimeType>& other) const = default;
};

} // namespace snail::analysis::detail

template<typename IdType, typename TimeType>
struct std::hash<snail::analysis::detail::id_at<IdType, TimeType>>
{
    [[nodiscard]] inline size_t operator()(const snail::analysis::detail::id_at<IdType, TimeType>& key) const noexcept
    {
        return snail::common::hash_combine(std::hash<IdType>()(key.id), std::hash<TimeType>()(key.time));
    }
};
