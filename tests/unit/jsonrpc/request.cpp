#include <gtest/gtest.h>

#include <snail/jsonrpc/request.hpp>

using namespace snail;

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
            {"str", false},
            {"num", 123  }
        };

        EXPECT_THROW(
            snail::jsonrpc::detail::unpack_request<test_2_request>(data),
            snail::jsonrpc::invalid_parameters_error);
    }
}
