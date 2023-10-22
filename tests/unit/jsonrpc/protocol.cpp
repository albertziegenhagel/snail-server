#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <snail/jsonrpc/errors.hpp>
#include <snail/jsonrpc/jsonrpc_v2_protocol.hpp>

using namespace snail::jsonrpc;

using namespace std::string_literals;

namespace {

// to suppress discarding return value warnings
void load_wrapper(v2_protocol& protocol, std::string_view content)
{
    [[maybe_unused]] const auto result = protocol.load_request(content);
}

} // namespace

TEST(JsonRpcV2Protocol, LoadRequest)
{
    v2_protocol protocol;

    {
        const auto request = protocol.load_request(R"({"jsonrpc":"2.0","method":"myCall"})");
        EXPECT_EQ(request.method, "myCall");
        EXPECT_EQ(request.params, nlohmann::json());
        EXPECT_EQ(request.id, std::nullopt);
    }
    {
        const auto request = protocol.load_request(R"({"jsonrpc":"2.0","method":"myCall","id":"my-id"})");
        EXPECT_EQ(request.method, "myCall");
        EXPECT_EQ(request.params, nlohmann::json());
        EXPECT_EQ(request.id, nlohmann::json("my-id"s));
    }
    {
        const auto request = protocol.load_request(R"({"jsonrpc":"2.0","method":"myCall","id":"my-id","params":{}})");
        EXPECT_EQ(request.method, "myCall");
        EXPECT_EQ(request.params, nlohmann::json::object());
        EXPECT_EQ(request.id, nlohmann::json("my-id"s));
    }
    {
        const auto request = protocol.load_request(R"({"jsonrpc":"2.0","method":"myCall","id":"my-id","params":{"my-param-1":false, "my-param-2":123}})");
        EXPECT_EQ(request.method, "myCall");
        EXPECT_EQ(request.params, (nlohmann::json{
                                      {"my-param-1"s, false},
                                      {"my-param-2"s, 123  }
        }));
        EXPECT_EQ(request.id, nlohmann::json("my-id"s));
    }
    {
        const auto request = protocol.load_request(R"({"jsonrpc":"2.0","method":"myCall","id":"my-id","params":[123, "text"]})");
        EXPECT_EQ(request.method, "myCall");
        EXPECT_EQ(request.params, (nlohmann::json{
                                      123, "text"s}));
        EXPECT_EQ(request.id, nlohmann::json("my-id"s));
    }
}

TEST(JsonRpcV2Protocol, LoadRequestInvalid)
{
    v2_protocol protocol;

    EXPECT_THROW(load_wrapper(protocol, R"()"),
                 parse_error);

    EXPECT_THROW(load_wrapper(protocol, R"(some stuff that is not valid json)"),
                 parse_error);

    EXPECT_THROW(load_wrapper(protocol, R"({})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"(123)"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"1.0"})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0"})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"method":"myCall"})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"1.0", "method":"myCall"})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":false})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":true, "method":"myCall"})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "unknown":null})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "params":123})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "params":null})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "id":true})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "id":true, "unknown":123})"),
                 invalid_request_error);

    EXPECT_THROW(load_wrapper(protocol, R"({"jsonrpc":"2.0", "method":"myCall", "params":{}, "id":"my-id", "unknown":{}})"),
                 invalid_request_error);
}

TEST(JsonRpcV2Protocol, DumpResponse)
{
    v2_protocol protocol;

    EXPECT_EQ(protocol.dump_response(response{
                  .result = nlohmann::json::object(),
                  .id     = {}}),
              R"({"jsonrpc":"2.0","result":{}})");

    EXPECT_EQ(protocol.dump_response(response{
                  .result = nlohmann::json::object(),
                  .id     = nlohmann::json("my-id"s)}),
              R"({"jsonrpc":"2.0","result":{},"id":"my-id"})");

    const auto result_data = (nlohmann::json{
        {"a", false},
        {"b", 123  }
    });
    EXPECT_EQ(protocol.dump_response(response{
                  .result = result_data,
                  .id     = nlohmann::json("my-id"s)}),
              R"({"jsonrpc":"2.0","result":{"a":false,"b":123},"id":"my-id"})");
}

TEST(JsonRpcV2Protocol, DumpError)
{
    v2_protocol protocol;

    const auto id = nlohmann::json("my-id"s);

    EXPECT_EQ(protocol.dump_error(rpc_error(123, "my error"), nullptr),
              R"({"jsonrpc":"2.0","error":{"code":123,"message":"my error"}})");

    EXPECT_EQ(protocol.dump_error(rpc_error(123, "my error"), &id),
              R"({"jsonrpc":"2.0","error":{"code":123,"message":"my error"},"id":"my-id"})");

    EXPECT_EQ(protocol.dump_error(rpc_error(123, "error messages \"needs\\escaping\""), nullptr),
              "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":123,\"message\":\"error messages \\\"needs\\\\escaping\\\"\"}}");

    EXPECT_EQ(protocol.dump_error(rpc_error(123, "error messages \"needs\\escaping\""), &id),
              "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":123,\"message\":\"error messages \\\"needs\\\\escaping\\\"\"},\"id\":\"my-id\"}");
}
