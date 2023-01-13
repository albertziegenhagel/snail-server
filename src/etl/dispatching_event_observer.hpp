
#pragma once

#include <variant>
#include <functional>
#include <unordered_map>
#include <concepts>

#include "etl/etlfile.hpp"
#include "etl/guid.hpp"

#include "etl/parser/trace_headers/system_trace.hpp"
#include "etl/parser/trace_headers/compact_trace.hpp"
#include "etl/parser/trace_headers/perfinfo_trace.hpp"
#include "etl/parser/trace_headers/instance_trace.hpp"
#include "etl/parser/trace_headers/full_header_trace.hpp"
#include "etl/parser/trace_headers/event_header_trace.hpp"

namespace snail::etl::detail {

struct group_handler_key
{
    parser::event_trace_group group;
    std::uint8_t type;
    std::uint16_t version;
    
    bool operator==(const group_handler_key& other) const noexcept
    {
        return group == other.group &&
            type == other.type &&
            version == other.version;
    }
};

struct guid_handler_key
{
    etl::guid guid;
    std::uint8_t type;
    std::uint16_t version;
    
    bool operator==(const guid_handler_key& other) const noexcept
    {
        return guid == other.guid &&
            type == other.type &&
            version == other.version;
    }
};

} // namespace snail::etl::detail


namespace std {

template<>
struct hash<snail::etl::detail::group_handler_key>
{
    std::size_t operator()(const snail::etl::detail::group_handler_key& key) const noexcept
    {
        std::hash<std::uint32_t> hash;
        return hash((std::uint32_t(key.group) << 24) |
                    (std::uint32_t(key.type)  << 16) |
                     std::uint32_t(key.version));
    }
};

template<>
struct hash<snail::etl::detail::guid_handler_key>
{
    std::size_t operator()(const snail::etl::detail::guid_handler_key& key) const noexcept
    {
        std::hash<snail::etl::guid> guid_hash;
        std::hash<std::uint32_t> int_hash;
        return guid_hash(key.guid) ^ int_hash((std::uint32_t(key.type) << 16) | std::uint32_t(key.version));
    }
};

} // namespace std

namespace snail::etl {

using any_group_trace_header = std::variant<
    parser::system_trace_header_view,
    parser::compact_trace_header_view,
    parser::perfinfo_trace_header_view
>;

using any_guid_trace_header = std::variant<
    parser::instance_trace_header_view,
    parser::full_header_trace_header_view
>;

struct common_trace_header
{
    std::uint8_t  type;
    std::uint64_t timestamp;
};

template<typename T>
concept event_record_view = std::constructible_from<T, std::span<const std::byte>, std::uint32_t>;

// template<typename T, typename EventType>
// concept group_event_handler = std::invocable<T, any_group_trace_header, EventType> ||
//                               std::invocable<T, common_trace_header, EventType>;
                              
// template<typename T, typename EventType>
// concept guid_event_handler = std::invocable<T, any_guid_trace_header, EventType> ||
//                              std::invocable<T, common_trace_header, EventType>;

template<typename T, typename EventType>
concept event_handler = std::invocable<T, any_group_trace_header, EventType> ||
                        std::invocable<T, any_guid_trace_header, EventType> ||
                        std::invocable<T, common_trace_header, EventType>;

class dispatching_event_observer : public event_observer
{
public:
    virtual void handle(const etl_file::header_data& file_header,
                        const parser::system_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override;

    virtual void handle(const etl_file::header_data& file_header,
                        const parser::compact_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override;

    virtual void handle(const etl_file::header_data& file_header,
                        const parser::perfinfo_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override;

    virtual void handle(const etl_file::header_data& file_header,
                        const parser::instance_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override;

    virtual void handle(const etl_file::header_data& file_header,
                        const parser::full_header_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override;


    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(parser::event_trace_group group, std::uint8_t type, std::uint8_t version,
                               HandlerType&& handler);

    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(const etl::guid& guid, std::uint8_t type, std::uint8_t version,
                               HandlerType&& handler);

    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(HandlerType&& handler);

private:
    using group_handler_dispatch_type = std::function<void(const etl_file::header_data&, const any_group_trace_header&, std::span<const std::byte>)>;
    using guid_handler_dispatch_type  = std::function<void(const etl_file::header_data&, const any_guid_trace_header&, std::span<const std::byte>)>;
    
    std::unordered_map<detail::group_handler_key, std::vector<group_handler_dispatch_type>> group_handlers_;
    std::unordered_map<detail::guid_handler_key, std::vector<guid_handler_dispatch_type>>   guid_handlers_;

    static common_trace_header make_common_trace_header(const any_group_trace_header& trace_header_variant);
    static common_trace_header make_common_trace_header(const any_guid_trace_header& trace_header_variant);
};

template<typename EventType, typename HandlerType>
    requires event_record_view<EventType> && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(etl::parser::event_trace_group group, std::uint8_t type, std::uint8_t version,
                                                       HandlerType&& handler)
{
    const auto key = detail::group_handler_key{group, type, version};

    if constexpr(std::invocable<HandlerType, any_group_trace_header, EventType>)
    {
        group_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl::etl_file::header_data& file_header,
                                                           const any_group_trace_header& trace_header_variant,
                                                           std::span<const std::byte> user_data) {
                const auto event = EventType(user_data, file_header.pointer_size);
                std::invoke(handler, trace_header_variant, event);
            });
    }
    else
    {
        static_assert(std::invocable<HandlerType, common_trace_header, EventType>);

        group_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl::etl_file::header_data& file_header,
                                                           const any_group_trace_header& trace_header_variant,
                                                           std::span<const std::byte> user_data) {
                const auto event         = EventType(user_data, file_header.pointer_size);
                const auto common_header = make_common_trace_header(trace_header_variant);
                std::invoke(handler, common_header, event);
            });
    }
}

template<typename EventType, typename HandlerType>
    requires event_record_view<EventType> && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(const etl::guid& guid, std::uint8_t type, std::uint8_t version,
                                                       HandlerType&& handler)
{
    const auto key = detail::guid_handler_key{guid, type, version};

    if constexpr(std::invocable<HandlerType, any_guid_trace_header, EventType>)
    {
        guid_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl::etl_file::header_data& file_header,
                                                           const any_guid_trace_header& trace_header_variant,
                                                           std::span<const std::byte> user_data) {
                const auto event = EventType(user_data, file_header.pointer_size);
                std::invoke(handler, trace_header_variant, event);
            });
    }
    else
    {
        static_assert(std::invocable<HandlerType, common_trace_header, EventType>);

        guid_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl::etl_file::header_data& file_header,
                                                           const any_guid_trace_header& trace_header_variant,
                                                           std::span<const std::byte> user_data) {
                const auto event         = EventType(user_data, file_header.pointer_size);
                const auto common_header = make_common_trace_header(trace_header_variant);
                std::invoke(handler, common_header, event);
            });
    }
}

template<typename EventType, typename HandlerType>
    requires event_record_view<EventType> && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(HandlerType&& handler)
{
    for(const auto& [group_or_guid, type, name] : EventType::event_types)
    {
        register_event<EventType>(group_or_guid, type, EventType::event_version, handler);
    }
}

} // namespace snail::etl
