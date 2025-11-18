#include "handler/static_file_handler.hpp"
#include "utest/utest.h"

#include <iostream>

// NOTE(isma): Might not be the best idea to hardcode these responses because
// the order of headers will likely change. If any of these tests fail, feel
// free to skip them until the HTTP response design is more stable.

UTEST(GetHandlerTest, StatusOk)
{
    char buf[4096] = {0};
    StaticFileHandler test("www/example/index.html");

    test.read_data(buf, sizeof(buf));

    ASSERT_STREQ(buf, "HTTP/1.1 200 OK\r\n"
                      "Connection: keep-alive\r\n"
                      "Content-Length: 64\r\n"
                      "Content-Type: text/html\r\n"
                      "\r\n"
                      "<!DOCTYPE html><html><head><title>Example</title></head></html>\n");
}

UTEST(GetHandlerTest, StatusNotFound)
{
    char buf[4096] = {0};
    StaticFileHandler test("www/example/inexistant.html");

    test.read_data(buf, sizeof(buf));

    ASSERT_STREQ(buf, "HTTP/1.1 404 Not Found\r\n\r\n");
}

UTEST(GetHandlerTest, ReadSomeData)
{
    char buf[4096] = {0};
    StaticFileHandler test("");

    EXPECT_TRUE(test.has_output());
    EXPECT_FALSE(test.needs_input());
    ASSERT_TRUE(test.read_data(buf, sizeof(buf)));
}

UTEST(GetHandlerTest, WriteNoData)
{
    char msg[] = "Hello, world!\n";
    StaticFileHandler test("");

    EXPECT_TRUE(test.has_output());
    EXPECT_FALSE(test.needs_input());
    ASSERT_FALSE(test.write_data(msg, sizeof(msg)));
}

UTEST(GetHandlerTest, SmallBuffer)
{
    char buf[1] = {0};
    StaticFileHandler test("www/example/index.html");

    std::string res;

    while (test.read_data(buf, sizeof(buf)) > 0) {
        res.append(buf, 1);
    }

    ASSERT_STREQ(res.c_str(), "HTTP/1.1 200 OK\r\n"
                              "Connection: keep-alive\r\n"
                              "Content-Length: 64\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n"
                              "<!DOCTYPE html><html><head><title>Example</title></head></html>\n");
}
