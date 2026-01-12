
#include "config/server_config.hpp"
#include "handler/delete_handler.hpp"
#include "handler/upload_handler.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

#include <iostream>
/*
static void print_response_expected(const std::string& response, const std::string& expected)
{
    std::cout << "reponse = " << std::endl << response << std::endl;
    std::cout << "expected = " << std::endl << expected << std::endl;
    std::cout << "response size = " << response.size() << std::endl;
    std::cout << "expected size = " << expected.size() << std::endl;
}
*/

static bool str_contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

static void write_all(Handler* handler, const std::string& content)
{
    size_t written = 0;

    while (handler->needs_input()) {
        size_t n = handler->write_input(content.c_str() + written, content.size() - written);
        written += n;

        if ((n == 0 && handler->needs_input()))
            break;
    }
}

UTEST(DeleteHandlerTest, file_not_found)
{
    std::string file_path = "www/example/delete_testing/file_not_found.txt";
    RouteConfig rc;
    HttpRequest req;
    DeleteHandler handler(file_path, rc, req);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    EXPECT_TRUE(str_contains(http_res, "HTTP/1.1 404"));
}

UTEST(DeleteHandlerTest, file_deleted_no_content)
{
    std::string file_path = "www/example/delete_testing/file_deleted_no_content.txt";
    {
        std::string content = "";
        RouteConfig rc;
        HttpRequest req;
        UploadHandler handler(file_path, rc, req);
        // write_all(handler, content);
    }
    RouteConfig rc;
    HttpRequest req;
    DeleteHandler handler(file_path, rc, req);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    std::string expected =
        HttpResponse::make_error(HttpResponse::kStatusNotFound, rc.shared.error_pages, req)
            .to_string();
    // print_response_expected(http_res, expected);
    ASSERT_TRUE(http_res == expected);
}

UTEST(DeleteHandlerTest, file_deleted)
{
    std::string file_path = "www/example/delete_testing/file_deleted.txt";
    {
        std::string content = "some content";
        RouteConfig rc;
        HttpRequest req;
        DeleteHandler handler(file_path, rc, req);
        write_all(&handler, content);
    }
    RouteConfig rc;
    HttpRequest req;
    DeleteHandler handler(file_path, rc, req);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    EXPECT_TRUE(str_contains(http_res, "HTTP/1.1 404 Not Found"));
}
