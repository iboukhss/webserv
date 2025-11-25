#include "http/http_parser.hpp"
#include "utest/utest.h"

UTEST(HttpParserTest, FullRequestLine)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.1\r\n";

    ASSERT_TRUE(p.status() == HttpParser::kIncomplete);

    p.feed_data(req.data(), req.size());
    ASSERT_TRUE(p.status() == HttpParser::kRequestLineDone);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == "HTTP/1.1");
}

UTEST(HttpParserTest, InvalidMethod)
{
    HttpParser p;
    std::string req = "GET/index.html HTTP/1.1\r\n";

    ASSERT_TRUE(p.status() == HttpParser::kIncomplete);

    p.feed_data(req.data(), req.size());
    ASSERT_TRUE(p.status() == HttpParser::kError);
}

UTEST(HttpParserTest, PartialFeed)
{
    HttpParser p;
    std::string req = "GET /index.html HTTP/1.1\r\n";

    ASSERT_TRUE(p.status() == HttpParser::kIncomplete);

    size_t crlf = req.find("\r\n");

    p.feed_data(req.data(), crlf);
    ASSERT_TRUE(p.status() == HttpParser::kIncomplete);

    p.feed_data(req.data() + crlf, 2);
    ASSERT_TRUE(p.status() == HttpParser::kRequestLineDone);

    EXPECT_TRUE(p.request().method == "GET");
    EXPECT_TRUE(p.request().path == "/index.html");
    EXPECT_TRUE(p.request().http_version == "HTTP/1.1");
}
