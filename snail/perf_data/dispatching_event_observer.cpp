
#include <snail/perf_data/dispatching_event_observer.hpp>

#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/parser/event_attributes.hpp>

using namespace snail::perf_data;

void dispatching_event_observer::handle(const parser::event_header_view& event_header,
                                        const parser::event_attributes&  attributes,
                                        std::span<const std::byte>       event_data,
                                        std::endian                      byte_order)
{
    auto iter = kernel_handlers_.find(static_cast<std::uint32_t>(event_header.type()));
    if(iter == kernel_handlers_.end()) return;

    for(const auto& handler : iter->second)
    {
        handler(attributes, event_data, byte_order);
    }
}

void dispatching_event_observer::handle(const parser::event_header_view& event_header,
                                        std::span<const std::byte>       event_data,
                                        std::endian                      byte_order)
{
    auto iter = non_kernel_handlers_.find(static_cast<std::uint32_t>(event_header.type()));
    if(iter == non_kernel_handlers_.end()) return;

    for(const auto& handler : iter->second)
    {
        handler(event_data, byte_order);
    }
}
