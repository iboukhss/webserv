#include "utest/utest.h"
#include "util/string.hpp"

#include <stdexcept>

UTEST(StrSplitTest, EmptyString)
{
    std::vector<std::string> v = str_split("", " ");
    ASSERT_EQ(v.size(), 1u);
    EXPECT_STREQ(v[0].c_str(), "");
}

UTEST(StrSplitTest, NoDelimiter)
{
    std::vector<std::string> v = str_split("hello", " ");
    ASSERT_EQ(v.size(), 1u);
    EXPECT_STREQ(v[0].c_str(), "hello");
}

UTEST(StrSplitTest, OneDelimiter)
{
    std::vector<std::string> v = str_split("hello world", " ");
    ASSERT_EQ(v.size(), 2u);
    EXPECT_STREQ(v[0].c_str(), "hello");
    EXPECT_STREQ(v[1].c_str(), "world");
}

UTEST(StrSplitTest, MultiCharDelimiter)
{
    std::vector<std::string> v = str_split("a==b==c", "==");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_STREQ(v[0].c_str(), "a");
    EXPECT_STREQ(v[1].c_str(), "b");
    EXPECT_STREQ(v[2].c_str(), "c");
}

UTEST(StrSplitTest, LeadingDelimiter)
{
    std::vector<std::string> v = str_split(" a b c", " ");
    ASSERT_EQ(v.size(), 4u);
    EXPECT_STREQ(v[0].c_str(), "");
    EXPECT_STREQ(v[1].c_str(), "a");
    EXPECT_STREQ(v[2].c_str(), "b");
    EXPECT_STREQ(v[3].c_str(), "c");
}

UTEST(StrSplitTest, TrailingDelimiter)
{
    std::vector<std::string> v = str_split("a b c ", " ");
    ASSERT_EQ(v.size(), 4u);
    EXPECT_STREQ(v[0].c_str(), "a");
    EXPECT_STREQ(v[1].c_str(), "b");
    EXPECT_STREQ(v[2].c_str(), "c");
    EXPECT_STREQ(v[3].c_str(), "");
}

UTEST(StrSplitTest, OnlyDelimiters)
{
    std::vector<std::string> v = str_split("   ", " ");
    ASSERT_EQ(v.size(), 4u); // ["", "", "", ""] (3 spaces == 4 tokens)
    EXPECT_STREQ(v[0].c_str(), "");
    EXPECT_STREQ(v[1].c_str(), "");
    EXPECT_STREQ(v[2].c_str(), "");
    EXPECT_STREQ(v[3].c_str(), "");
}

UTEST(StrSplitTest, EmptyDelimiter)
{
    std::vector<std::string> v;
    EXPECT_EXCEPTION(str_split("hello world", ""), std::invalid_argument);
}

UTEST(StrSplitTest, HttpRequestLine)
{
    std::vector<std::string> v = str_split("GET / HTTP/1.1", " ");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_STREQ(v[0].c_str(), "GET");
    EXPECT_STREQ(v[1].c_str(), "/");
    EXPECT_STREQ(v[2].c_str(), "HTTP/1.1");
}

UTEST(StrSplitTest, HttpResponse)
{
    std::string s = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 14\r\n"
                    "\r\n"
                    "Hello, world!\n";

    std::vector<std::string> v = str_split(s, "\r\n");

    ASSERT_EQ(v.size(), 5u);
    EXPECT_STREQ(v[0].c_str(), "HTTP/1.1 200 OK");
    EXPECT_STREQ(v[1].c_str(), "Content-Type: text/plain");
    EXPECT_STREQ(v[2].c_str(), "Content-Length: 14");
    EXPECT_STREQ(v[3].c_str(), "");
    EXPECT_STREQ(v[4].c_str(), "Hello, world!\n");
}
