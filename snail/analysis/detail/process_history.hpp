#pragma once

#include <concepts>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace snail::analysis::detail {

template<std::integral Id, std::integral Timestamp, typename Data>
    requires std::equality_comparable<Data>
class history
{
public:
    struct entry
    {
        Id        id;
        Timestamp timestamp;

        Data payload;
    };

    void insert(Id id, Timestamp timestamp, Data payload);

    template<typename Self>
    [[nodiscard]] auto* find_at(this Self&& self, Id id, Timestamp timestamp, bool strict = false);

    [[nodiscard]] const std::unordered_map<Id, std::vector<entry>>& all_entries() const
    {
        return entries_by_id;
    }

private:
    std::unordered_map<Id, std::vector<entry>> entries_by_id;
};

template<std::integral Id, std::integral Timestamp, typename Data>
    requires std::equality_comparable<Data>
void history<Id, Timestamp, Data>::insert(Id id, Timestamp timestamp, Data payload)
{
    auto& entries = entries_by_id[id];

    if(entries.empty() || entries.back().timestamp <= timestamp)
    {
        if(!entries.empty() && entries.back().payload == payload) return;

        entries.push_back(entry{
            .id        = id,
            .timestamp = timestamp,
            .payload   = std::move(payload)});
    }
    else
    {
        assert(false); // not yet implemented
    }
}

template<std::integral Id, std::integral Timestamp, typename Data>
    requires std::equality_comparable<Data>
template<typename Self>
auto* history<Id, Timestamp, Data>::find_at(this Self&& self, Id id, Timestamp timestamp, bool strict)
{
    using return_type = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const entry*, entry*>;

    auto iter = self.entries_by_id.find(id);
    if(iter == self.entries_by_id.end()) return (return_type) nullptr;

    // entries are sorted: latest entry comes last.
    for(auto& entry : std::views::reverse(iter->second))
    {
        if(entry.timestamp <= timestamp) return &entry;
    }

    if(strict) return (return_type) nullptr;

    return &iter->second.back();
}

} // namespace snail::analysis::detail
