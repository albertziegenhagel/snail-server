#pragma once

#include <cstdint>

#include <functional>

namespace snail::analysis {

struct unique_process_id
{
    std::uint64_t key;

    [[nodiscard]] inline auto operator<=>(const unique_process_id& other) const = default;
};

struct unique_thread_id
{
    std::uint64_t key;

    [[nodiscard]] inline auto operator<=>(const unique_thread_id& other) const = default;
};

} // namespace snail::analysis

template<>
struct std::hash<snail::analysis::unique_process_id>
{
    [[nodiscard]] inline size_t operator()(const snail::analysis::unique_process_id& id) const noexcept
    {
        return std::hash<std::uint64_t>()(id.key);
    }
};

template<>
struct std::hash<snail::analysis::unique_thread_id>
{
    [[nodiscard]] inline size_t operator()(const snail::analysis::unique_thread_id& id) const noexcept
    {
        return std::hash<std::uint64_t>()(id.key);
    }
};
