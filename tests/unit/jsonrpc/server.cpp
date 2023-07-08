#include <gtest/gtest.h>

#include <snail/jsonrpc/server.hpp>

#include <snail/jsonrpc/request.hpp>

#include <snail/jsonrpc/transport/stream_message_reader.hpp>
#include <snail/jsonrpc/transport/stream_message_writer.hpp>

#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/jsonrpc_v2_protocol.hpp>

using namespace snail;
using namespace snail::jsonrpc;

SNAIL_JSONRPC_REQUEST_2(my_test_1,
                        std::string_view, str,
                        int, num)

SNAIL_JSONRPC_REQUEST_1(my_test_2,
                        std::string_view, str)

SNAIL_JSONRPC_REQUEST_0(my_test_3)

TEST(JsonRpcServer, ServeSome)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    jsonrpc::server server(
        std::make_unique<message_connection>(
            std::make_unique<stream_message_reader>(in_stream),
            std::make_unique<stream_message_writer>(out_stream)),
        std::make_unique<v2_protocol>());

    std::string test_1_str;
    int         test_1_num;
    server.register_request<my_test_1_request>(
        [&](const my_test_1_request& request) -> nlohmann::json
        {
            test_1_str = request.str();
            test_1_num = request.num();
            return nlohmann::json("response string");
        });

    std::string test_2_str;
    server.register_notification<my_test_2_request>(
        [&](const my_test_2_request& request)
        {
            test_2_str = request.str();
        });

    in_stream << "Content-Length:83\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_1","id":1,"params":{"str":"something","num":42}})";

    in_stream << "Content-Length:72\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_2","params":{"str":"something else"}})";

    server.serve_next();

    EXPECT_EQ(test_1_str, "something");
    EXPECT_EQ(test_1_num, 42);
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})");

    server.serve_next();

    EXPECT_EQ(test_2_str, "something else");
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})");

    in_stream << "Content-Length:64\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"does-not-exist","params":{"num":123}})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})"
              "Content-Length: 68\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"does-not-exist"}})");

    server.register_request<my_test_3_request>(
        [&](const my_test_3_request&) -> nlohmann::json
        {
            throw std::runtime_error("unexpected");
        });

    in_stream << "Content-Length:45\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_3","id":3})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})"
              "Content-Length: 68\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"does-not-exist"}})"
              "Content-Length: 71\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32603,"message":"unexpected"},"id":3})");
}
