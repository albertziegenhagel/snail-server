
#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <unordered_map>
#include <variant>

#include <snail/etl/etl_file.hpp>

#include <snail/etl/parser/trace_headers/compact_trace.hpp>
#include <snail/etl/parser/trace_headers/event_header_trace.hpp>
#include <snail/etl/parser/trace_headers/full_header_trace.hpp>
#include <snail/etl/parser/trace_headers/instance_trace.hpp>
#include <snail/etl/parser/trace_headers/perfinfo_trace.hpp>
#include <snail/etl/parser/trace_headers/system_trace.hpp>

#include <snail/common/guid.hpp>
#include <snail/common/hash_combine.hpp>

namespace snail::etl::detail {

struct group_handler_key
{
    parser::event_trace_group group;
    std::uint8_t              type;
    std::uint16_t             version;

    bool operator==(const group_handler_key& other) const noexcept
    {
        return group == other.group &&
               type == other.type &&
               version == other.version;
    }
};

struct guid_handler_key
{
    common::guid  guid;
    std::uint16_t type;
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
                    (std::uint32_t(key.type) << 16) |
                    std::uint32_t(key.version));
    }
};

template<>
struct hash<snail::etl::detail::guid_handler_key>
{
    std::size_t operator()(const snail::etl::detail::guid_handler_key& key) const noexcept
    {
        std::hash<snail::common::guid> guid_hash;
        std::hash<std::uint32_t>       int_hash;
        return snail::common::hash_combine(guid_hash(key.guid), int_hash((std::uint32_t(key.type) << 16) | std::uint32_t(key.version)));
    }
};

} // namespace std

namespace snail::etl {

using any_group_trace_header = std::variant<
    parser::system_trace_header_view,
    parser::compact_trace_header_view,
    parser::perfinfo_trace_header_view>;

using any_guid_trace_header = std::variant<
    parser::instance_trace_header_view,
    parser::full_header_trace_header_view,
    parser::event_header_trace_header_view>;

struct common_trace_header
{
    std::uint16_t              type;
    std::uint64_t              timestamp;
    std::span<const std::byte> buffer;
};

common_trace_header make_common_trace_header(const any_group_trace_header& trace_header_variant);
common_trace_header make_common_trace_header(const any_guid_trace_header& trace_header_variant);

template<typename T>
concept event_record_view = std::constructible_from<T, std::span<const std::byte>, std::uint32_t>;

template<typename T, typename EventType>
concept event_handler = std::invocable<T, etl_file::header_data, any_group_trace_header, EventType> ||
                        std::invocable<T, etl_file::header_data, any_guid_trace_header, EventType> ||
                        std::invocable<T, etl_file::header_data, common_trace_header, EventType>;

template<typename T>
concept unknown_event_handler = std::invocable<T, etl_file::header_data, any_group_trace_header, std::span<const std::byte>> ||
                                std::invocable<T, etl_file::header_data, any_guid_trace_header, std::span<const std::byte>> ||
                                std::invocable<T, etl_file::header_data, common_trace_header, std::span<const std::byte>>;

class dispatching_event_observer : public event_observer
{
public:
    virtual void handle(const etl_file::header_data&            file_header,
                        const parser::system_trace_header_view& trace_header,
                        std::span<const std::byte>              user_data) override;

    virtual void handle(const etl_file::header_data&             file_header,
                        const parser::compact_trace_header_view& trace_header,
                        std::span<const std::byte>               user_data) override;

    virtual void handle(const etl_file::header_data&              file_header,
                        const parser::perfinfo_trace_header_view& trace_header,
                        std::span<const std::byte>                user_data) override;

    virtual void handle(const etl_file::header_data&              file_header,
                        const parser::instance_trace_header_view& trace_header,
                        std::span<const std::byte>                user_data) override;

    virtual void handle(const etl_file::header_data&                 file_header,
                        const parser::full_header_trace_header_view& trace_header,
                        std::span<const std::byte>                   user_data) override;

    virtual void handle(const etl_file::header_data&                  file_header,
                        const parser::event_header_trace_header_view& trace_header,
                        std::span<const std::byte>                    user_data) override;

    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(parser::event_trace_group group, std::uint8_t type, std::uint8_t version,
                               HandlerType&& handler);

    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(const common::guid& guid, std::uint16_t type, std::uint8_t version,
                               HandlerType&& handler);

    template<typename EventType, typename HandlerType>
        requires event_record_view<EventType> && event_handler<HandlerType, EventType>
    inline void register_event(HandlerType&& handler);

    template<typename HandlerType>
        requires unknown_event_handler<HandlerType>
    inline void register_unknown_event(HandlerType&& handler);

protected:
    virtual void pre_handle([[maybe_unused]] const etl_file::header_data&     file_header,
                            [[maybe_unused]] const detail::group_handler_key& key,
                            [[maybe_unused]] const any_group_trace_header&    trace_header,
                            [[maybe_unused]] std::span<const std::byte>       user_data,
                            [[maybe_unused]] bool                             has_known_handler) {}
    virtual void pre_handle([[maybe_unused]] const etl_file::header_data&    file_header,
                            [[maybe_unused]] const detail::guid_handler_key& key,
                            [[maybe_unused]] const any_guid_trace_header&    trace_header,
                            [[maybe_unused]] std::span<const std::byte>      user_data,
                            [[maybe_unused]] bool                            has_known_handler) {}

private:
    using group_handler_dispatch_type = std::function<void(const etl_file::header_data&, const any_group_trace_header&, std::span<const std::byte>)>;
    using guid_handler_dispatch_type  = std::function<void(const etl_file::header_data&, const any_guid_trace_header&, std::span<const std::byte>)>;

    std::unordered_map<detail::group_handler_key, std::vector<group_handler_dispatch_type>> group_handlers_;
    std::unordered_map<detail::guid_handler_key, std::vector<guid_handler_dispatch_type>>   guid_handlers_;

    std::vector<group_handler_dispatch_type> unknown_group_handlers_;
    std::vector<guid_handler_dispatch_type>  unknown_guid_handlers_;

    template<typename HeaderType, typename HandlersType, typename UnknownHandlersType>
    void handle_impl(const etl_file::header_data& file_header,
                     const HeaderType&            trace_header,
                     std::span<const std::byte>   user_data,
                     const HandlersType&          handlers,
                     const UnknownHandlersType&   unknown_handlers);
};

template<typename EventType, typename HandlerType>
    requires event_record_view<EventType> && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(etl::parser::event_trace_group group, std::uint8_t type, std::uint8_t version,
                                                       HandlerType&& handler)
{
    const auto key = detail::group_handler_key{group, type, version};

    if constexpr(std::invocable<HandlerType, etl_file::header_data, any_group_trace_header, EventType>)
    {
        group_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data&  file_header,
                                                           const any_group_trace_header& trace_header_variant,
                                                           std::span<const std::byte>    user_data)
            {
                const auto event = EventType(user_data, file_header.pointer_size);
                std::invoke(handler, file_header, trace_header_variant, event);
            });
    }
    else
    {
        static_assert(std::invocable<HandlerType, etl_file::header_data, common_trace_header, EventType>);

        group_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data&  file_header,
                                                           const any_group_trace_header& trace_header_variant,
                                                           std::span<const std::byte>    user_data)
            {
                const auto event         = EventType(user_data, file_header.pointer_size);
                const auto common_header = make_common_trace_header(trace_header_variant);
                std::invoke(handler, file_header, common_header, event);
            });
    }
}

template<typename EventType, typename HandlerType>
    requires event_record_view<EventType> && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(const common::guid& guid, std::uint16_t type, std::uint8_t version,
                                                       HandlerType&& handler)
{
    const auto key = detail::guid_handler_key{guid, type, version};

    if constexpr(std::invocable<HandlerType, etl_file::header_data, any_guid_trace_header, EventType>)
    {
        guid_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data& file_header,
                                                           const any_guid_trace_header& trace_header_variant,
                                                           std::span<const std::byte>   user_data)
            {
                const auto event = EventType(user_data, file_header.pointer_size);
                assert(event.dynamic_size() == event.buffer().size());
                std::invoke(handler, file_header, trace_header_variant, event);
            });
    }
    else
    {
        static_assert(std::invocable<HandlerType, etl_file::header_data, common_trace_header, EventType>);

        guid_handlers_[key].push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data& file_header,
                                                           const any_guid_trace_header& trace_header_variant,
                                                           std::span<const std::byte>   user_data)
            {
                const auto event = EventType(user_data, file_header.pointer_size);
                assert(event.dynamic_size() == event.buffer().size());
                const auto common_header = make_common_trace_header(trace_header_variant);
                std::invoke(handler, file_header, common_header, event);
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

template<typename HandlerType>
    requires unknown_event_handler<HandlerType>
inline void dispatching_event_observer::register_unknown_event(HandlerType&& handler)
{
    if constexpr(std::invocable<HandlerType, etl_file::header_data, any_group_trace_header, std::span<const std::byte>>)
    {
        unknown_group_handlers_.push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data&  file_header,
                                                           const any_group_trace_header& trace_header_variant,
                                                           std::span<const std::byte>    user_data)
            {
                std::invoke(handler, file_header, trace_header_variant, user_data);
            });
    }
    if constexpr(std::invocable<HandlerType, etl_file::header_data, any_guid_trace_header, std::span<const std::byte>>)
    {
        unknown_guid_handlers_.push_back(
            [handler = std::forward<HandlerType>(handler)](const etl_file::header_data& file_header,
                                                           const any_guid_trace_header& trace_header_variant,
                                                           std::span<const std::byte>   user_data)
            {
                std::invoke(handler, file_header, trace_header_variant, user_data);
            });
    }
    if constexpr(std::invocable<HandlerType, etl_file::header_data, common_trace_header, std::span<const std::byte>>)
    {
        const auto shared_handle_holder = std::make_shared(std::forward<HandlerType>(handler));

        unknown_group_handlers_.push_back(
            [handle_holder = shared_handle_holder](const etl_file::header_data&  file_header,
                                                   const any_group_trace_header& trace_header_variant,
                                                   std::span<const std::byte>    user_data)
            {
                std::invoke(*handle_holder, file_header, trace_header_variant, user_data);
            });
        unknown_guid_handlers_.push_back(
            [handle_holder = shared_handle_holder](const etl_file::header_data& file_header,
                                                   const any_guid_trace_header& trace_header_variant,
                                                   std::span<const std::byte>   user_data)
            {
                const auto common_header = make_common_trace_header(trace_header_variant);
                std::invoke(*handle_holder, file_header, common_header, user_data);
            });
    }
}

} // namespace snail::etl
