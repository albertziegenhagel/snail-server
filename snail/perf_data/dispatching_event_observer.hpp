
#pragma once

#include <concepts>
#include <functional>
#include <unordered_map>
#include <variant>

#include <snail/common/detail/dump.hpp>

#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data {

template<typename T>
concept event_record_view =
    std::is_base_of_v<parser::event_view_base, T> &&
    (std::constructible_from<T, std::span<const std::byte>, std::endian> ||
     std::constructible_from<T, const parser::event_attributes&, std::span<const std::byte>, std::endian>);

template<typename T>
concept parsable_event_record = requires(const parser::event_attributes& attributes,
                                         std::span<const std::byte>      buffer,
                                         std::endian                     byte_order) {
    {
        parser::parse_event<T>(attributes, buffer, byte_order)
    } -> std::same_as<T>;
};

template<typename T, typename EventType>
concept event_handler = std::invocable<T, EventType>;

class dispatching_event_observer : public event_observer
{
public:
    virtual void handle(const parser::event_header_view& event_header,
                        const parser::event_attributes&  attributes,
                        std::span<const std::byte>       event_data,
                        std::endian                      byte_order) override;

    template<typename EventType, typename HandlerType>
        requires(event_record_view<EventType> || parsable_event_record<EventType>) && event_handler<HandlerType, EventType>
    inline void register_event(std::uint32_t event_type, HandlerType&& handler);

    template<typename EventType, typename HandlerType>
        requires(event_record_view<EventType> || parsable_event_record<EventType>) && event_handler<HandlerType, EventType>
    inline void register_event(HandlerType&& handler);

private:
    using handler_dispatch_type = std::function<void(const parser::event_attributes&, std::span<const std::byte>, std::endian)>;

    std::unordered_map<std::uint32_t, std::vector<handler_dispatch_type>> handlers_;
};

template<typename EventType, typename HandlerType>
    requires(event_record_view<EventType> || parsable_event_record<EventType>) && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(std::uint32_t event_type, HandlerType&& handler)
{
    if constexpr(event_record_view<EventType>)
    {
        handlers_[event_type].push_back(
            [handler = std::forward<HandlerType>(handler)]([[maybe_unused]] const parser::event_attributes& attributes,
                                                           std::span<const std::byte>                       event_data,
                                                           std::endian                                      byte_order)
            {
                if constexpr(std::constructible_from<EventType, std::span<const std::byte>, std::endian>)
                {
                    const auto event = EventType(event_data, byte_order);
                    std::invoke(handler, event);
                }
                else
                {
                    const auto event = EventType(attributes, event_data, byte_order);
                    std::invoke(handler, event);
                }
            });
    }
    else
    {
        handlers_[event_type].push_back(
            [handler = std::forward<HandlerType>(handler)](const parser::event_attributes& attributes,
                                                           std::span<const std::byte>      event_data,
                                                           std::endian                     byte_order)
            {
                const auto event = parser::parse_event<EventType>(attributes, event_data, byte_order);
                std::invoke(handler, event);
            });
    }
}

template<typename EventType, typename HandlerType>
    requires(event_record_view<EventType> || parsable_event_record<EventType>) && event_handler<HandlerType, EventType>
inline void dispatching_event_observer::register_event(HandlerType&& handler)
{
    register_event<EventType>(static_cast<std::uint32_t>(EventType::event_type), handler);
}

} // namespace snail::perf_data
