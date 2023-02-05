#pragma once

#include <string>
#include <tuple>

#include <snail/jsonrpc/detail/request.hpp>

#define SNAIL_JSONRPC_REQUEST_0(request_name)                                                      \
    struct request_name##_request                                                                  \
    {                                                                                              \
        static constexpr std::string_view name = #request_name;                                    \
                                                                                                   \
        static constexpr std::tuple<> parameters = {};                                             \
                                                                                                   \
        template<typename RequestType>                                                             \
            requires is_request_v<RequestType>                                                     \
        friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data); \
                                                                                                   \
    private:                                                                                       \
        std::tuple<> data_;                                                                        \
    };                                                                                             \
    namespace snail::jsonrpc::detail {                                                             \
    template<>                                                                                     \
    struct is_request<request_name##_request> : std::true_type                                     \
    {};                                                                                            \
    }

#define SNAIL_JSONRPC_REQUEST_1(request_name,                                                      \
                                param_0_type, param_0_name)                                        \
    struct request_name##_request                                                                  \
    {                                                                                              \
        static constexpr std::string_view name = #request_name;                                    \
                                                                                                   \
        static constexpr auto parameters = std::tuple(                                             \
            snail::jsonrpc::detail::request_parameter<param_0_type>{#param_0_name});               \
                                                                                                   \
        const param_0_type& param_0_name() const                                                   \
        {                                                                                          \
            return std::get<0>(data_);                                                             \
        }                                                                                          \
                                                                                                   \
        template<typename RequestType>                                                             \
            requires is_request_v<RequestType>                                                     \
        friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data); \
                                                                                                   \
    private:                                                                                       \
        std::tuple<param_0_type>                                                                   \
            data_;                                                                                 \
    };                                                                                             \
    namespace snail::jsonrpc::detail {                                                             \
    template<>                                                                                     \
    struct is_request<request_name##_request> : std::true_type                                     \
    {};                                                                                            \
    }

#define SNAIL_JSONRPC_REQUEST_2(request_name,                                                      \
                                param_0_type, param_0_name,                                        \
                                param_1_type, param_1_name)                                        \
    struct request_name##_request                                                                  \
    {                                                                                              \
        static constexpr std::string_view name = #request_name;                                    \
                                                                                                   \
        static constexpr auto parameters = std::tuple(                                             \
            snail::jsonrpc::detail::request_parameter<param_0_type>{#param_0_name},                \
            snail::jsonrpc::detail::request_parameter<param_1_type>{#param_1_name});               \
                                                                                                   \
        const param_0_type& param_0_name() const                                                   \
        {                                                                                          \
            return std::get<0>(data_);                                                             \
        }                                                                                          \
        const param_1_type& param_1_name() const                                                   \
        {                                                                                          \
            return std::get<1>(data_);                                                             \
        }                                                                                          \
                                                                                                   \
        template<typename RequestType>                                                             \
            requires is_request_v<RequestType>                                                     \
        friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data); \
                                                                                                   \
    private:                                                                                       \
        std::tuple<param_0_type,                                                                   \
                   param_1_type>                                                                   \
            data_;                                                                                 \
    };                                                                                             \
    namespace snail::jsonrpc::detail {                                                             \
    template<>                                                                                     \
    struct is_request<request_name##_request> : std::true_type                                     \
    {};                                                                                            \
    }

#define SNAIL_JSONRPC_REQUEST_3(request_name,                                                      \
                                param_0_type, param_0_name,                                        \
                                param_1_type, param_1_name,                                        \
                                param_2_type, param_2_name)                                        \
    struct request_name##_request                                                                  \
    {                                                                                              \
        static constexpr std::string_view name = #request_name;                                    \
                                                                                                   \
        static constexpr auto parameters = std::tuple(                                             \
            snail::jsonrpc::detail::request_parameter<param_0_type>{#param_0_name},                \
            snail::jsonrpc::detail::request_parameter<param_1_type>{#param_1_name},                \
            snail::jsonrpc::detail::request_parameter<param_2_type>{#param_2_name});               \
                                                                                                   \
        const param_0_type& param_0_name() const                                                   \
        {                                                                                          \
            return std::get<0>(data_);                                                             \
        }                                                                                          \
        const param_1_type& param_1_name() const                                                   \
        {                                                                                          \
            return std::get<1>(data_);                                                             \
        }                                                                                          \
        const param_2_type& param_2_name() const                                                   \
        {                                                                                          \
            return std::get<2>(data_);                                                             \
        }                                                                                          \
                                                                                                   \
        template<typename RequestType>                                                             \
            requires is_request_v<RequestType>                                                     \
        friend RequestType snail::jsonrpc::detail::unpack_request(const nlohmann::json& raw_data); \
                                                                                                   \
    private:                                                                                       \
        std::tuple<param_0_type,                                                                   \
                   param_1_type,                                                                   \
                   param_2_type>                                                                   \
            data_;                                                                                 \
    };                                                                                             \
    namespace snail::jsonrpc::detail {                                                             \
    template<>                                                                                     \
    struct is_request<request_name##_request> : std::true_type                                     \
    {};                                                                                            \
    }
