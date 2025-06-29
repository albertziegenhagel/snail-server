#include <gtest/gtest.h>

#include <snail/jsonrpc/request.hpp>

using namespace snail;

enum class my_int_test_enum : unsigned int
{
    a = 15,
    b = 5
};

enum class my_str_test_enum
{
    a,
    b
};
namespace snail::jsonrpc::detail {
template<>
struct enum_value_type<my_str_test_enum>
{
    using type = std::string_view;
};
template<>
my_str_test_enum enum_from_value<my_str_test_enum>(const std::string_view& value)
{
    if(value == "a") return my_str_test_enum::a;
    if(value == "b") return my_str_test_enum::b;
    throw std::runtime_error(std::format("'{}' is not a valid value for enum 'my_str_test_enum'", value));
}

} // namespace snail::jsonrpc::detail

SNAIL_JSONRPC_REQUEST_0(empty)

SNAIL_JSONRPC_REQUEST_1(test_1,
                        std::string_view, str)

SNAIL_JSONRPC_REQUEST_2(test_2,
                        std::string_view, str,
                        int, num)

SNAIL_JSONRPC_REQUEST_3(test_3,
                        std::string_view, str,
                        int, num,
                        bool, flag)

SNAIL_JSONRPC_REQUEST_2(test_4,
                        my_int_test_enum, int_enum,
                        my_str_test_enum, str_enum)

SNAIL_JSONRPC_REQUEST_1(test_5,
                        std::optional<int>, opt_num)

using int_or_str_variant = std::variant<int, std::string>;
SNAIL_JSONRPC_REQUEST_1(test_6,
                        int_or_str_variant, int_or_str)

using uint_or_str_variant = std::variant<unsigned int, std::string>;
SNAIL_JSONRPC_REQUEST_1(test_7,
                        uint_or_str_variant, uint_or_str)

using bool_or_double_variant = std::variant<bool, double>;
SNAIL_JSONRPC_REQUEST_1(test_8,
                        bool_or_double_variant, bool_or_double)

using str_or_float_variant = std::variant<std::string, float>;
SNAIL_JSONRPC_REQUEST_1(test_9,
                        str_or_float_variant, str_or_float)

using str_variant = std::variant<std::string>;
SNAIL_JSONRPC_REQUEST_1(test_10,
                        str_variant, str)

TEST(JsonRpc, UnpackRequest)
{
    {
        const nlohmann::json data = {};

        [[maybe_unused]] const auto request = snail::jsonrpc::detail::unpack_request<empty_request>(data);
    }

    {
        const nlohmann::json data = {
            {"str", "test_str"}
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_1_request>(data);

        EXPECT_EQ(request.str(), "test_str");
    }

    {
        const nlohmann::json data = {
            {"str", "test_str"},
            {"num", 123       }
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_2_request>(data);

        EXPECT_EQ(request.str(), "test_str");
        EXPECT_EQ(request.num(), 123);
    }

    {
        const nlohmann::json data = {
            {"str",  "test_str"},
            {"num",  123       },
            {"flag", false     }
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_3_request>(data);

        EXPECT_EQ(request.str(), "test_str");
        EXPECT_EQ(request.num(), 123);
        EXPECT_EQ(request.flag(), false);
    }

    {
        const nlohmann::json data = {
            {"str", "test_str"},
            {"num", 123       }
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_3_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"str", false}, // invalid value type
            {"num", 123  }
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_2_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"int_enum", 15 },
            {"str_enum", "a"}
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_4_request>(data);

        EXPECT_EQ(request.int_enum(), my_int_test_enum::a);
        EXPECT_EQ(request.str_enum(), my_str_test_enum::a);
    }

    {
        const nlohmann::json data = {
            {"int_enum", 123},
            {"str_enum", "b"}
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_4_request>(data);

        EXPECT_EQ(request.int_enum(), my_int_test_enum(123)); // int can be outside
        EXPECT_EQ(request.str_enum(), my_str_test_enum::b);
    }

    {
        const nlohmann::json data = {
            {"int_enum", 5  },
            {"str_enum", "c"}  // invalid enum value
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_4_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"opt_num", 15},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_5_request>(data);

        ASSERT_NE(request.opt_num(), std::nullopt);
        EXPECT_EQ(*request.opt_num(), 15);
    }

    {
        const nlohmann::json data = {
            {"opt_num", nullptr},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_5_request>(data);

        EXPECT_EQ(request.opt_num(), std::nullopt);
    }

    {
        const nlohmann::json data = {};

        const auto request = snail::jsonrpc::detail::unpack_request<test_5_request>(data);

        EXPECT_EQ(request.opt_num(), std::nullopt);
    }

    {
        const nlohmann::json data = {
            {"int_or_str", 15},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_6_request>(data);

        EXPECT_EQ(request.int_or_str(), int_or_str_variant(15));
    }

    {
        const nlohmann::json data = {
            {"int_or_str", 15u},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_6_request>(data);

        EXPECT_EQ(request.int_or_str(), int_or_str_variant(15));
    }

    {
        const nlohmann::json data = {
            {"int_or_str", "test_str"},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_6_request>(data);

        EXPECT_EQ(request.int_or_str(), int_or_str_variant("test_str"));
    }

    {
        const nlohmann::json data = {
            {"uint_or_str", 15},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_7_request>(data);

        EXPECT_EQ(request.uint_or_str(), uint_or_str_variant(15u));
    }

    {
        const nlohmann::json data = {
            {"uint_or_str", 15u},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_7_request>(data);

        EXPECT_EQ(request.uint_or_str(), uint_or_str_variant(15u));
    }

    {
        const nlohmann::json data = {
            {"bool_or_double", true},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_8_request>(data);

        EXPECT_EQ(request.bool_or_double(), bool_or_double_variant(true));
    }

    {
        const nlohmann::json data = {
            {"bool_or_double", 1.23},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_8_request>(data);

        EXPECT_EQ(request.bool_or_double(), bool_or_double_variant(1.23));
    }

    {
        const nlohmann::json data = {
            {"bool_or_double", 1.23f},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_8_request>(data);

        EXPECT_EQ(request.bool_or_double(), bool_or_double_variant(1.23f));
    }

    {
        const nlohmann::json data = {
            {"bool_or_double", "test_str"},
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_8_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"str_or_float", 1.23},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_9_request>(data);

        EXPECT_EQ(request.str_or_float(), str_or_float_variant(1.23f));
    }

    {
        const nlohmann::json data = {
            {"str_or_float", 1.23f},
        };

        const auto request = snail::jsonrpc::detail::unpack_request<test_9_request>(data);

        EXPECT_EQ(request.str_or_float(), str_or_float_variant(1.23f));
    }

    {
        const nlohmann::json data = {
            {"str", true},
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_10_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"str", 15},
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_10_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"str", 15u},
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_10_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }

    {
        const nlohmann::json data = {
            {"str", 1.23},
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_10_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }
}
