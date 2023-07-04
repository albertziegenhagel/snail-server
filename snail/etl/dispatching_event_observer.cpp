
#include <concepts>

#include <snail/etl/dispatching_event_observer.hpp>

using namespace snail::etl;

namespace {

template<typename T>
    requires std::same_as<T, parser::system_trace_header_view> ||
             std::same_as<T, parser::compact_trace_header_view> ||
             std::same_as<T, parser::perfinfo_trace_header_view>
auto make_key(const T& trace_header)
{
    return detail::group_handler_key{
        trace_header.packet().group(),
        trace_header.packet().type(),
        trace_header.version()};
}

template<typename T>
    requires std::same_as<T, parser::full_header_trace_header_view> ||
             std::same_as<T, parser::instance_trace_header_view>
auto make_key(const T& trace_header)
{
    return detail::guid_handler_key{
        trace_header.guid().instantiate(),
        trace_header.trace_class().type(),
        trace_header.trace_class().version()};
}

auto make_key(const parser::event_header_trace_header_view& trace_header)
{
    return detail::guid_handler_key{
        trace_header.provider_id().instantiate(),
        trace_header.event_descriptor().id(),
        trace_header.event_descriptor().version()};
}

template<typename T>
    requires std::same_as<T, parser::system_trace_header_view> ||
             std::same_as<T, parser::compact_trace_header_view> ||
             std::same_as<T, parser::perfinfo_trace_header_view>
auto make_header_variant(const T& trace_header)
{
    return any_group_trace_header{trace_header};
}

template<typename T>
    requires std::same_as<T, parser::full_header_trace_header_view> ||
             std::same_as<T, parser::instance_trace_header_view> ||
             std::same_as<T, parser::event_header_trace_header_view>
auto make_header_variant(const T& trace_header)
{
    return any_guid_trace_header{trace_header};
}

template<typename HeaderType, typename HandlersType, typename UnknownHandlersType>
void handle_impl(const etl_file::header_data& file_header,
                 const HeaderType&            trace_header,
                 std::span<const std::byte>   user_data,
                 const HandlersType&          handlers,
                 const UnknownHandlersType&   unknown_handlers)
{
    const auto key = make_key(trace_header);

    auto iter = handlers.find(key);
    if(iter == handlers.end())
    {
        if(!unknown_handlers.empty())
        {
            const auto trace_header_variant = make_header_variant(trace_header);
            for(const auto& handler : unknown_handlers)
            {
                handler(file_header, trace_header_variant, user_data);
            }
        }
        return;
    }

    const auto trace_header_variant = make_header_variant(trace_header);
    for(const auto& handler : iter->second)
    {
        handler(file_header, trace_header_variant, user_data);
    }
}

} // namespace

common_trace_header dispatching_event_observer::make_common_trace_header(const any_group_trace_header& trace_header_variant)
{
    return std::visit(
        [](const auto& trace_header)
        {
            return common_trace_header{
                .type      = trace_header.packet().type(),
                .timestamp = trace_header.system_time()};
        },
        trace_header_variant);
}

common_trace_header dispatching_event_observer::make_common_trace_header(const any_guid_trace_header& trace_header_variant)
{
    return std::visit(
        []<typename T>(const T& trace_header)
        {
            if constexpr(std::is_same_v<T, parser::event_header_trace_header_view>)
            {
                return common_trace_header{
                    .type      = trace_header.event_descriptor().id(),
                    .timestamp = trace_header.timestamp()};
            }
            else
            {
                return common_trace_header{
                    .type      = trace_header.trace_class().type(),
                    .timestamp = trace_header.timestamp()};
            }
        },
        trace_header_variant);
}

void dispatching_event_observer::handle(const etl_file::header_data&            file_header,
                                        const parser::system_trace_header_view& trace_header,
                                        std::span<const std::byte>              user_data)
{
    handle_impl(file_header, trace_header, user_data, group_handlers_, unknown_group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&             file_header,
                                        const parser::compact_trace_header_view& trace_header,
                                        std::span<const std::byte>               user_data)
{
    handle_impl(file_header, trace_header, user_data, group_handlers_, unknown_group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&              file_header,
                                        const parser::perfinfo_trace_header_view& trace_header,
                                        std::span<const std::byte>                user_data)
{
    handle_impl(file_header, trace_header, user_data, group_handlers_, unknown_group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&                  file_header,
                                        const parser::event_header_trace_header_view& trace_header,
                                        std::span<const std::byte>                    user_data)
{
    handle_impl(file_header, trace_header, user_data, guid_handlers_, unknown_guid_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&              file_header,
                                        const parser::instance_trace_header_view& trace_header,
                                        std::span<const std::byte>                user_data)
{
    handle_impl(file_header, trace_header, user_data, guid_handlers_, unknown_guid_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&                 file_header,
                                        const parser::full_header_trace_header_view& trace_header,
                                        std::span<const std::byte>                   user_data)
{
    handle_impl(file_header, trace_header, user_data, guid_handlers_, unknown_guid_handlers_);
}
