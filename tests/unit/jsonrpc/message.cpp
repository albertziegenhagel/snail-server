#include <gtest/gtest.h>

#include <sstream>

#include <snail/jsonrpc/transport/stream_message_reader.hpp>
#include <snail/jsonrpc/transport/stream_message_writer.hpp>

#include <snail/jsonrpc/transport/message_connection.hpp>

#include <snail/jsonrpc/message_handler.hpp>

#include <snail/jsonrpc/errors.hpp>

using namespace snail::jsonrpc;

struct test_message_handler : public message_handler
{
    void handle(std::string data, respond_callback respond) override
    {
        request = std::move(data);
        if(response) respond(*response);
    }

    std::string                request;
    std::optional<std::string> response;
};

TEST(StreamMessageReader, Read)
{
    std::stringstream     stream(std::ios::in | std::ios::out | std::ios::binary);
    stream_message_reader message_reader(stream);

    stream << "Content-Length:19\n"
           << "\r\n"
           << "{\"my-message\": 123}";

    EXPECT_EQ(message_reader.read(), "{\"my-message\": 123}");

    stream << "Content-Length:     18\r\n"
           << "\r\n"
           << "some other message";

    EXPECT_EQ(message_reader.read(), "some other message");

    stream << "Content-Length: 19\n"
           << "\r\n"
           << "multi\n"
           << "line\n"
           << "message\n";

    EXPECT_EQ(message_reader.read(), "multi\nline\nmessage\n");

    stream << "Content-Length: 7\n"
           << "Content-Type: application/vscode-jsonrpc;charset=utf-8\n"
           << "\r\n"
           << "message";

    EXPECT_EQ(message_reader.read(), "message");

    stream << "Content-Length: 11\n"
           << "\n"
           << "\r\n"
           << "👻message";

    EXPECT_EQ(message_reader.read(), "👻message");
}

TEST(StreamMessageReader, ReadInvalid)
{
    std::stringstream     stream(std::ios::in | std::ios::out | std::ios::binary);
    stream_message_reader message_reader(stream);

    stream << "Content-Length:xx\n"
           << "\r\n"
           << "{\"my-message\": 123}";

    EXPECT_THROW(message_reader.read(),
                 transport_error);

    stream.str("");
    stream.clear();

    stream << "Content-Length: 19\n"
           << "Invalid-Header: XXX\n"
           << "\r\n"
           << "{\"my-message\": 123}";

    EXPECT_THROW(message_reader.read(),
                 transport_error);
}

TEST(StreamMessageWriter, Write)
{
    std::stringstream     stream(std::ios::in | std::ios::out | std::ios::binary);
    stream_message_writer message_writer(stream);

    message_writer.write("{\"my-message\": 123}");

    EXPECT_EQ(stream.str(),
              "Content-Length: 19\r\n"
              "\r\n"
              "{\"my-message\": 123}");
}

TEST(MessageConnection, ServeSome)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    message_connection connection(
        std::make_unique<stream_message_reader>(in_stream),
        std::make_unique<stream_message_writer>(out_stream));

    test_message_handler handler;

    handler.response = "my-response";

    in_stream << "Content-Length: 10\r\n"
              << "\r\n"
              << "my-request";

    connection.serve_next(handler);

    EXPECT_EQ(handler.request, "my-request");
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 11\r\n"
              "\r\n"
              "my-response");

    handler.request.clear();
    handler.response.reset();

    in_stream << "Content-Length: 12\r\n"
              << "\r\n"
              << "my-request-2";

    connection.serve_next(handler);

    EXPECT_EQ(handler.request, "my-request-2");
    EXPECT_EQ(out_stream.str(),
              "Content-Length: 11\r\n"
              "\r\n"
              "my-response");
}

TEST(MessageConnection, Send)
{
    std::stringstream in_stream(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream out_stream(std::ios::in | std::ios::out | std::ios::binary);

    message_connection connection(
        std::make_unique<stream_message_reader>(in_stream),
        std::make_unique<stream_message_writer>(out_stream));

    connection.send("{\"my-message\": 123}");

    EXPECT_EQ(out_stream.str(),
              "Content-Length: 19\r\n"
              "\r\n"
              "{\"my-message\": 123}");
}
