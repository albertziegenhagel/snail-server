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

SNAIL_JSONRPC_REQUEST_0(my_test_4)

TEST(JsonRpcServer, ServeValidRequestAndNotification)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    jsonrpc::server server(
        std::make_unique<message_connection>(
            std::make_unique<stream_message_reader>(in_stream),
            std::make_unique<stream_message_writer>(out_stream)),
        std::make_unique<v2_protocol>());

    request_id  test_1_id;
    std::string test_1_str;
    int         test_1_num;
    server.register_request<my_test_1_request>(
        [&](const request_id&         request_id,
            const my_test_1_request&  request,
            request_response_callback respond,
            error_callback /*report_error*/)
        {
            test_1_id  = request_id;
            test_1_str = request.str();
            test_1_num = request.num();
            respond(nlohmann::json("response string"));
        });

    std::string test_2_str;
    server.register_notification<my_test_2_request>(
        [&](const my_test_2_request& request,
            error_callback /*report_error*/)
        {
            test_2_str = request.str();
        });

    // Test a request and a notification , where the notification arrives
    // in the stream before the server handled the request.

    in_stream << "Content-Length:83\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_1","id":1,"params":{"str":"something","num":42}})";

    in_stream << "Content-Length:72\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_2","params":{"str":"something else"}})";

    // handle the request
    server.serve_next();

    EXPECT_EQ(test_1_id, request_id(1));
    EXPECT_EQ(test_1_str, "something");
    EXPECT_EQ(test_1_num, 42);
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})");

    // and handle the notification (which does not result in any response)
    server.serve_next();

    EXPECT_EQ(test_2_str, "something else");
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 51\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","result":"response string","id":1})");
}

TEST(JsonRpcServer, ServeInvalid)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    jsonrpc::server server(
        std::make_unique<message_connection>(
            std::make_unique<stream_message_reader>(in_stream),
            std::make_unique<stream_message_writer>(out_stream)),
        std::make_unique<v2_protocol>());

    // Invalid json
    in_stream << "Content-Length:23\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","metho)";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 245\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"[json.exception.parse_error.101] parse error at line 1, column 24: syntax error while parsing object key - invalid string: missing closing quote; last read: '\"metho'; expected string literal"}})");

    // Invalid request (no jsonrpc)
    in_stream << "Content-Length:38\r\n"
              << "\r\n"
              << R"({"method":"my_test_2"})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 245\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"[json.exception.parse_error.101] parse error at line 1, column 24: syntax error while parsing object key - invalid string: missing closing quote; last read: '\"metho'; expected string literal"}})"
              "Content-Length: 71\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32600,"message":"'jsonrpc' missing"}})");
}

TEST(JsonRpcServer, ServeUnknown)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    jsonrpc::server server(
        std::make_unique<message_connection>(
            std::make_unique<stream_message_reader>(in_stream),
            std::make_unique<stream_message_writer>(out_stream)),
        std::make_unique<v2_protocol>());

    // Test unknown notification
    in_stream << "Content-Length:75\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"non-existing-notification","params":{"num":123}})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 110\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"Unknown notification method: 'non-existing-notification'"}})");

    // Test unknown request
    in_stream << "Content-Length:77\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"non-existing-request","id":2,"params":{"num":123}})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 110\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"Unknown notification method: 'non-existing-notification'"}})"
              "Content-Length: 107\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"Unknown request method: 'non-existing-request'"},"id":2})");
}

TEST(JsonRpcServer, UnexpectedError)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    jsonrpc::server server(
        std::make_unique<message_connection>(
            std::make_unique<stream_message_reader>(in_stream),
            std::make_unique<stream_message_writer>(out_stream)),
        std::make_unique<v2_protocol>());

    // Test request that reports an unexpected error.
    server.register_request<my_test_3_request>(
        [&](const request_id&,
            const my_test_3_request&,
            request_response_callback /*respond*/,
            error_callback report_error)
        {
            report_error("unexpected in request");
        });

    in_stream << "Content-Length:45\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_3","id":3})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 82\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32603,"message":"unexpected in request"},"id":3})");

    // And test a notification with an unexpected error.
    // Actually, I am not sure whether we should send a response here according to the spec?
    server.register_notification<my_test_4_request>(
        [&](const my_test_4_request&,
            error_callback report_error)
        {
            report_error("unexpected in notification");
        });

    in_stream << "Content-Length:38\r\n"
              << "\r\n"
              << R"({"jsonrpc":"2.0","method":"my_test_4"})";

    server.serve_next();

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 82\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32603,"message":"unexpected in request"},"id":3})"
              "Content-Length: 80\r\n"
              "\r\n"
              R"({"jsonrpc":"2.0","error":{"code":-32603,"message":"unexpected in notification"}})");
}
