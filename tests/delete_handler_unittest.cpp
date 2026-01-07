
#include "handler/delete_handler.hpp"
#include "handler/upload_handler.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

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
    DeleteHandler handler(file_path);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    ASSERT_TRUE(http_res.find("404") != std::string::npos);
}

UTEST(DeleteHandlerTest, file_deleted_no_content)
{
    std::string file_path = "www/example/delete_testing/file_deleted_no_content.txt";
    {
        std::string content = "";
        UploadHandler handler(file_path, content.size());
        // write_all(handler, content);
    }
    DeleteHandler handler(file_path);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    ASSERT_TRUE(http_res.find("204") != std::string::npos);
}

UTEST(DeleteHandlerTest, file_deleted)
{
    std::string file_path = "www/example/delete_testing/file_deleted.txt";
    {
        std::string content = "some content";
        UploadHandler handler(file_path, content.size());
        write_all(&handler, content);
    }
    DeleteHandler handler(file_path);
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    ASSERT_TRUE(http_res.find("200") != std::string::npos);
}
