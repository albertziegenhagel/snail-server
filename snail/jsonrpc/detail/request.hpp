#pragma once

#include <format>
#include <string>
#include <tuple>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/errors.hpp>

namespace snail::jsonrpc::detail {

template<typename T>
struct request_parameter
{
    using type = T;
    std::string_view name;
};

template<typename T>
struct is_request : std::false_type
{};

template<typename T>
inline constexpr bool is_request_v = is_request<T>::value;

template<typename... Ts>
constexpr auto make_index_sequence(const std::tuple<Ts...>&)
{
    return std::index_sequence_for<Ts...>();
}

template<std::size_t I>
using size_c = std::integral_constant<std::size_t, I>;

template<typename F, typename... Ts, std::size_t... Is>
constexpr void for_each(const std::tuple<Ts...>& data, [[maybe_unused]] F func, std::index_sequence<Is...>)
{
    (func(size_c<Is>(), std::get<Is>(data)), ...);
}

template<typename RequestType>
    requires is_request_v<RequestType>
RequestType unpack_request(const nlohmann::json& raw_data)
{
    RequestType result;

    for_each(
        RequestType::parameters,
        [&raw_data, &result]<std::size_t I, typename T>(size_c<I>, const request_parameter<T>& parameter)
        {
            auto iter = raw_data.find(parameter.name);
            if(iter == raw_data.end()) throw invalid_parameters_error(std::format("Missing parameter: {}", parameter.name).c_str());

            try
            {
                iter->get_to(std::get<I>(result.data_));
            }
            catch(const nlohmann::json::type_error& e)
            {
                throw invalid_parameters_error(std::format("Invalid parameter type: {}", e.what()).c_str());
            }
        },
        make_index_sequence(RequestType::parameters));

    return result;
}

} // namespace snail::jsonrpc::detail
