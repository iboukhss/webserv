#include "config/server_config.hpp"
#include "handler/static_file_handler.hpp"
#include "http/http_version.hpp"
#include "utest/utest.h"

#include <iostream>

// NOTE(isma): Might not be the best idea to hardcode these responses because
// the order of headers will likely change. If any of these tests fail, feel
// free to skip them until the HTTP response design is more stable.

static bool str_contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

UTEST(StaticFileHandlerTest, StatusOk)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("www/example/index.html", rc, req);

    char buf[4096] = {0};
    size_t n = test.read_output(buf, sizeof(buf));
    std::string response(buf, n);

    EXPECT_TRUE(str_contains(response, "HTTP/1.1 200 OK"));
    EXPECT_TRUE(str_contains(response, "Content-Length: 64"));
    EXPECT_TRUE(str_contains(response, "<!DOCTYPE html>"
                                       "<html><head><title>Example</title></head></html>\n"));
}

UTEST(StaticFileHandlerTest, StatusNotFound)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("www/example/inexistant.html", rc, req);

    char buf[4096] = {0};
    size_t n = test.read_output(buf, sizeof(buf));
    std::string response(buf, n);

    EXPECT_TRUE(
        str_contains(response, "404 Not Found")); // I removed the HTTP version from the check
}

UTEST(StaticFileHandlerTest, ReadSomeData)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("", rc, req);

    char buf[4096] = {0};

    EXPECT_TRUE(test.has_output());
    EXPECT_FALSE(test.needs_input());
    ASSERT_TRUE(test.read_output(buf, sizeof(buf)));
}

UTEST(StaticFileHandlerTest, WriteNoData)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("", rc, req);
    char msg[] = "Hello, world!\n";

    EXPECT_TRUE(test.has_output());
    EXPECT_FALSE(test.needs_input());
    ASSERT_FALSE(test.write_input(msg, sizeof(msg)));
}

UTEST(StaticFileHandlerTest, SmallBuffer)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("www/example/index.html", rc, req);

    char buf[1] = {0};
    std::string response;

    while (test.read_output(buf, sizeof(buf)) > 0) {
        response.append(buf, 1);
    }

    EXPECT_TRUE(str_contains(response, "HTTP/1.1 200 OK"));
    EXPECT_TRUE(str_contains(response, "Content-Length: 64"));
    EXPECT_TRUE(str_contains(response, "<!DOCTYPE html>"
                                       "<html><head><title>Example</title></head></html>\n"));
}

UTEST(StaticFileHandlerTest, is_done)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("www/example/index.html", rc, req);

    char buf[1] = {0};
    std::string response;

    while (!test.is_done() && test.has_output()) {
        test.read_output(buf, sizeof(buf));
        response.append(buf, 1);
    }

    EXPECT_TRUE(str_contains(response, "HTTP/1.1 200 OK"));
    EXPECT_TRUE(str_contains(response, "Content-Length: 64"));
    EXPECT_TRUE(str_contains(response, "<!DOCTYPE html>"
                                       "<html><head><title>Example</title></head></html>\n"));
}
UTEST(StaticFileHandlerTest, needs_input)
{
    RouteConfig rc;
    HttpRequest req;
    StaticFileHandler test("www/example/index.html", rc, req);
    EXPECT_FALSE(test.needs_input());
}

UTEST(StaticFileHandlerTest, dir_autoindex_off_with_index_file)
{
    RouteConfig rc;
    rc.shared.autoindex_enabled = false;
    HttpRequest req;
    StaticFileHandler test("www/example/", rc, req);

    char buf[1] = {0};
    std::string response;

    while (!test.is_done() && test.has_output()) {
        test.read_output(buf, sizeof(buf));
        response.append(buf, 1);
    }
    // std::cout << response << std::endl;
    EXPECT_TRUE(str_contains(response, "HTTP/1.1 200 OK"));
}

UTEST(StaticFileHandlerTest, dir_autoindex_off_no_index_file)
{
    RouteConfig rc;
    rc.shared.autoindex_enabled = false;
    HttpRequest req;
    StaticFileHandler test("www/example/bin/", rc, req);

    char buf[1] = {0};
    std::string response;

    while (!test.is_done() && test.has_output()) {
        test.read_output(buf, sizeof(buf));
        response.append(buf, 1);
    }
    // std::cout << response << std::endl;
    EXPECT_TRUE(str_contains(response, "HTTP/1.1 403 Forbidden"));
}

UTEST(StaticFileHandlerTest, dir_autoindex_on_no_index_file)
{
    RouteConfig rc;
    rc.shared.autoindex_enabled = true;
    HttpRequest req;
    StaticFileHandler test("www/example/bin/", rc, req);

    char buf[1] = {0};
    std::string response;

    while (!test.is_done() && test.has_output()) {
        test.read_output(buf, sizeof(buf));
        response.append(buf, 1);
    }
    // std::cout << response << std::endl;
    EXPECT_TRUE(str_contains(response, "HTTP/1.1 200 OK"));
}

UTEST(StaticFileHandlerTest, dir_autoindex_on_no_index_file_redirection)
{
    RouteConfig rc;
    rc.shared.autoindex_enabled = true;
    HttpRequest req;
    StaticFileHandler test("www/example/bin", rc, req);

    char buf[1] = {0};
    std::string response;

    while (!test.is_done() && test.has_output()) {
        test.read_output(buf, sizeof(buf));
        response.append(buf, 1);
    }
    std::cout << response << std::endl;
    EXPECT_TRUE(str_contains(response, "HTTP/1.1 301 Moved Permanently"));
}
