#pragma once

#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <variant>

#include <nlohmann/json.hpp>

#include <snail/jsonrpc/errors.hpp>

namespace snail::jsonrpc::detail {

template<typename T>
struct to_tuple;

template<typename T>
using to_tuple_t = typename to_tuple<T>::type;

template<typename... Ts>
struct to_tuple<std::variant<Ts...>>
{
    using type = std::tuple<Ts...>;
};

template<typename, template<typename...> class>
struct filter;

template<typename... Ts,
         template<typename...> class Pred>
struct filter<std::tuple<Ts...>, Pred>
{
    using type = decltype(std::tuple_cat(std::declval<
                                         std::conditional_t<Pred<Ts>::value,
                                                            std::tuple<Ts>,
                                                            std::tuple<>>>()...));
};

template<typename Tpl,
         template<typename...> class Pred>
using filter_t = typename filter<Tpl, Pred>::type;

template<typename T>
struct is_integral_number
{
    static constexpr bool value = std::is_integral_v<T> && !std::is_same_v<T, bool>;
};

template<typename>
struct most_precise;

template<typename T>
using most_precise_t = typename most_precise<T>::type;

template<typename T, typename... Ts>
struct most_precise<std::tuple<T, Ts...>>
{
    using type = decltype((std::declval<T>() + ... + std::declval<Ts>()));
};

template<typename T, template<typename...> typename Template>
struct is_instantiation : std::false_type
{};

template<template<typename...> typename Template, typename... Args>
struct is_instantiation<Template<Args...>, Template> : std::true_type
{};

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

template<typename T>
    requires std::is_enum_v<T>
struct enum_value_type
{
    using type = std::underlying_type_t<T>;
};

template<typename T>
    requires std::is_enum_v<T>
using enum_value_type_t = typename enum_value_type<T>::type;

template<typename T>
    requires std::is_enum_v<T>
T enum_from_value(const enum_value_type_t<T>& value);

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

template<typename T, typename U>
void unpack_value(std::string_view parameter_name, U& dest, const nlohmann::json& raw_data, nlohmann::json::const_iterator& iter)
{
    if constexpr(is_instantiation<T, std::optional>{})
    {
        if(iter == raw_data.end() || iter->is_null())
        {
            dest = std::nullopt;
        }
        else
        {
            try
            {
                dest.emplace();
                unpack_value<std::decay_t<decltype(*dest)>>(parameter_name, *dest, raw_data, iter);
            }
            catch(const nlohmann::json::type_error& e)
            {
                throw invalid_parameters_error(std::format("Invalid parameter type: {}", e.what()).c_str());
            }
        }
    }
    else
    {
        if(iter == raw_data.end()) throw invalid_parameters_error(std::format("Missing parameter: {}", parameter_name).c_str());

        if constexpr(std::is_enum_v<T>)
        {
            enum_value_type_t<T> value;
            try
            {
                iter->get_to(value);
            }
            catch(const nlohmann::json::type_error& e)
            {
                throw invalid_parameters_error(std::format("Invalid parameter type: {}", e.what()).c_str());
            }
            if constexpr(std::is_integral_v<enum_value_type_t<T>>)
            {
                dest = static_cast<T>(value);
            }
            else
            {
                try
                {
                    dest = snail::jsonrpc::detail::enum_from_value<T>(value);
                }
                catch(const std::runtime_error& e)
                {
                    throw invalid_parameters_error(std::format("Invalid parameter value: {}", e.what()).c_str());
                }
            }
        }
        else if constexpr(is_instantiation<T, std::variant>{})
        {
            switch(iter->type())
            {
            case nlohmann::json::value_t::string:
                if constexpr(std::is_assignable_v<T, std::string>)
                {
                    dest = iter->template get<std::string>();
                }
                else
                {
                    throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
                }
                break;
            case nlohmann::json::value_t::boolean:
                if constexpr(std::is_assignable_v<T, bool>)
                {
                    dest = iter->template get<bool>();
                }
                else
                {
                    throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
                }
                break;
            case nlohmann::json::value_t::number_integer:
                if constexpr(std::is_assignable_v<T, std::int64_t>)
                {
                    dest = iter->template get<std::int64_t>();
                }
                else
                {
                    using integral_types = filter_t<to_tuple_t<T>, is_integral_number>;
                    using signed_types   = filter_t<integral_types, std::is_signed>;
                    if constexpr(std::tuple_size_v<signed_types> > 0)
                    {
                        dest = static_cast<most_precise_t<signed_types>>(iter->template get<std::int64_t>());
                    }
                    else if constexpr(std::tuple_size_v<integral_types> > 0)
                    {
                        dest = static_cast<most_precise_t<integral_types>>(iter->template get<std::int64_t>());
                    }
                    else
                    {
                        throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
                    }
                }
                break;
            case nlohmann::json::value_t::number_unsigned:
                if constexpr(std::is_assignable_v<T, std::uint64_t>)
                {
                    dest = iter->template get<std::uint64_t>();
                }
                else
                {
                    using integral_types = filter_t<to_tuple_t<T>, is_integral_number>;
                    using unsigned_types = filter_t<integral_types, std::is_unsigned>;
                    if constexpr(std::tuple_size_v<unsigned_types> > 0)
                    {
                        dest = static_cast<most_precise_t<unsigned_types>>(iter->template get<std::uint64_t>());
                    }
                    else if constexpr(std::tuple_size_v<integral_types> > 0)
                    {
                        dest = static_cast<most_precise_t<integral_types>>(iter->template get<std::uint64_t>());
                    }
                    else
                    {
                        throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
                    }
                }
                break;
            case nlohmann::json::value_t::number_float:
                if constexpr(std::is_assignable_v<T, double>)
                {
                    dest = iter->template get<double>();
                }
                else if constexpr(std::is_assignable_v<T, float>)
                {
                    dest = static_cast<float>(iter->template get<double>());
                }
                else
                {
                    throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
                }
                break;
            default:
                throw invalid_parameters_error(std::format("Invalid parameter type: {}", iter->type_name()).c_str());
            }
        }
        else
        {
            try
            {
                iter->get_to(dest);
            }
            catch(const nlohmann::json::type_error& e)
            {
                throw invalid_parameters_error(std::format("Invalid parameter type: {}", e.what()).c_str());
            }
        }
    }
}

template<typename RequestType>
    requires snail::jsonrpc::detail::is_request_v<RequestType>
RequestType unpack_request(const nlohmann::json& raw_data)
{
    RequestType result;

    for_each(
        RequestType::parameters,
        [&raw_data, &result]<std::size_t I, typename T>(size_c<I>, const request_parameter<T>& parameter)
        {
            auto iter = raw_data.find(parameter.name);

            unpack_value<T>(parameter.name, std::get<I>(result.data_), raw_data, iter);
        },
        make_index_sequence(RequestType::parameters));

    return result;
}

} // namespace snail::jsonrpc::detail
