#include "http/http_parser.hpp"
#include "utest/utest.h"

UTEST(HttpParserTest, BasicRequestLine)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.0\r\n";

    p.append_data(req.data(), req.size());
    ASSERT_TRUE(p.state() == HttpParser::kParsingHeaders);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == kHttpVersion1_0);
}

UTEST(HttpParserTest, FullRequestZeroBody)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.0\r\n"
                      "\r\n"
                      "\r\n";

    p.append_data(req.data(), req.size());
    ASSERT_TRUE(p.state() == HttpParser::kParsingDone);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == kHttpVersion1_0);
    EXPECT_TRUE(p.request().content_length == 0);
    EXPECT_FALSE(p.request().keep_alive);
}

UTEST(HttpParserTest, InvalidMethod)
{
    HttpParser p;
    std::string req = "PUT /index.html HTTP/1.0\r\n";

    p.append_data(req.data(), req.size());
    ASSERT_TRUE(p.state() == HttpParser::kParsingError);
}

UTEST(HttpParserTest, ImportantHeaders)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.1\r\n"
                      "Content-Length: 5\r\n"
                      "Connection: keep-alive\r\n"
                      "\r\n";

    p.append_data(req.data(), req.size());
    ASSERT_TRUE(p.state() == HttpParser::kParsingBody);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == kHttpVersion1_1);
    EXPECT_TRUE(p.request().content_length == 5);
    EXPECT_TRUE(p.request().keep_alive);
}

UTEST(HttpParserTest, PartialFeed)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.0\r\n";

    size_t crlf = req.find("\r\n");

    p.append_data(req.data(), crlf);
    ASSERT_TRUE(p.state() == HttpParser::kParsingRequestLine);

    p.append_data(req.data() + crlf, 2);
    ASSERT_TRUE(p.state() == HttpParser::kParsingHeaders);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == kHttpVersion1_0);
}

UTEST(HttpParserTest, ByteByByteFeed)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.0\r\n"
                      "\r\n"
                      "\r\n";

    for (size_t i = 0; i < req.size(); i++) {
        p.append_data(&req[i], 1);
    }

    ASSERT_TRUE(p.state() == HttpParser::kParsingDone);
    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == kHttpVersion1_0);
    EXPECT_TRUE(p.request().content_length == 0);
    EXPECT_FALSE(p.request().keep_alive);
}
