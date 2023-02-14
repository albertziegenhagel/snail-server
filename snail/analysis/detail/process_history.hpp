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

    [[nodiscard]] const entry* find_at(Id id, Timestamp timestamp, bool strict = false) const;
    [[nodiscard]] entry*       find_at(Id id, Timestamp timestamp, bool strict = false);

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
const typename history<Id, Timestamp, Data>::entry* history<Id, Timestamp, Data>::find_at(Id id, Timestamp timestamp, bool strict) const
{
    auto iter = entries_by_id.find(id);
    if(iter == entries_by_id.end()) return nullptr;

    // entries are sorted: latest entry comes last.
    for(auto& entry : std::views::reverse(iter->second))
    {
        if(entry.timestamp <= timestamp) return &entry;
    }

    if(strict) return nullptr;

    return &iter->second.back();
}

template<std::integral Id, std::integral Timestamp, typename Data>
    requires std::equality_comparable<Data>
typename history<Id, Timestamp, Data>::entry* history<Id, Timestamp, Data>::find_at(Id id, Timestamp timestamp, bool strict)
{
    const auto& const_self = *this;
    return const_cast<entry*>(const_self.find_at(id, timestamp, strict));
}

} // namespace snail::analysis::detail
