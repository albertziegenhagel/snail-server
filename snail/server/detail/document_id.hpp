#pragma once

#include <cstddef>

namespace snail::server::detail {

struct document_id
{
    std::size_t id_;

    [[nodiscard]] inline auto operator<=>(const document_id& other) const = default;
};

} // namespace snail::server::detail

template<>
struct std::hash<snail::server::detail::document_id>
{
    [[nodiscard]] inline size_t operator()(const snail::server::detail::document_id& id) const noexcept
    {
        return std::hash<std::size_t>()(id.id_);
    }
};
