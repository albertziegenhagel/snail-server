
#include <concepts>

#include <snail/etl/dispatching_event_observer.hpp>

using namespace snail::etl;

namespace {

// NOTE: These should be equal to
//         `dispatching_event_observer::group_handler_dispatch_type` and
//         `dispatching_event_observer::guid_handler_dispatch_type`
//       which we cannot access here since they is private.
using group_handler_dispatch_type_impl = std::function<void(const etl_file::header_data&, const any_group_trace_header&, std::span<const std::byte>)>;
using guid_handler_dispatch_type_impl  = std::function<void(const etl_file::header_data&, const any_guid_trace_header&, std::span<const std::byte>)>;

template<typename GroupHeaderType>
void handle_group_impl(const etl_file::header_data&                                                                        file_header,
                       const GroupHeaderType&                                                                              trace_header,
                       std::span<const std::byte>                                                                          user_data,
                       const std::unordered_map<detail::group_handler_key, std::vector<group_handler_dispatch_type_impl>>& handlers)
{
    const auto key = detail::group_handler_key{
        trace_header.packet().group(),
        trace_header.packet().type(),
        trace_header.version()};

    auto iter = handlers.find(key);
    if(iter == handlers.end()) return;

    const auto trace_header_variant = any_group_trace_header{trace_header};
    for(const auto& handler : iter->second)
    {
        handler(file_header, trace_header_variant, user_data);
    }
}

template<typename GuidHeaderType>
void handle_guid_impl(const etl_file::header_data&                                                                      file_header,
                      const GuidHeaderType&                                                                             trace_header,
                      std::span<const std::byte>                                                                        user_data,
                      const std::unordered_map<detail::guid_handler_key, std::vector<guid_handler_dispatch_type_impl>>& handlers)
{
    const auto key = detail::guid_handler_key{
        trace_header.guid().instantiate(),
        trace_header.trace_class().type(),
        trace_header.trace_class().version()};

    auto iter = handlers.find(key);
    if(iter == handlers.end()) return;

    const auto trace_header_variant = any_guid_trace_header{trace_header};
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
        [](const auto& trace_header)
        {
            return common_trace_header{
                .type      = trace_header.trace_class().type(),
                .timestamp = trace_header.timestamp()};
        },
        trace_header_variant);
}

void dispatching_event_observer::handle(const etl_file::header_data&            file_header,
                                        const parser::system_trace_header_view& trace_header,
                                        std::span<const std::byte>              user_data)
{
    handle_group_impl(file_header, trace_header, user_data, group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&             file_header,
                                        const parser::compact_trace_header_view& trace_header,
                                        std::span<const std::byte>               user_data)
{
    handle_group_impl(file_header, trace_header, user_data, group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&              file_header,
                                        const parser::perfinfo_trace_header_view& trace_header,
                                        std::span<const std::byte>                user_data)
{
    handle_group_impl(file_header, trace_header, user_data, group_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&              file_header,
                                        const parser::instance_trace_header_view& trace_header,
                                        std::span<const std::byte>                user_data)
{
    handle_guid_impl(file_header, trace_header, user_data, guid_handlers_);
}

void dispatching_event_observer::handle(const etl_file::header_data&                 file_header,
                                        const parser::full_header_trace_header_view& trace_header,
                                        std::span<const std::byte>                   user_data)
{
    handle_guid_impl(file_header, trace_header, user_data, guid_handlers_);
}
