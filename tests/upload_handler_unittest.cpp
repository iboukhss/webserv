#include "handler/upload_handler.hpp"
#include "utest/utest.h"

#include <cstdio>

static void write_all(Handler& handler, const std::string& content)
{
    size_t written = 0;

    while (handler.needs_input()) {
        size_t n = handler.write_input(content.c_str() + written, content.size() - written);
        written += n;

        if ((n == 0 && handler.needs_input()))
            break;
    }
}

UTEST(UploadHandlerTest, needs_input_upload_ongoing)
{
    std::string upload_path = "www/example/needs_input_upload_ongoing.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    ASSERT_TRUE(handler.needs_input());
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, has_output_upload_ongoing)
{
    std::string upload_path = "www/example/has_output_upload_ongoing.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    ASSERT_FALSE(handler.has_output());
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, needs_input_upload_done)
{
    std::string upload_path = "www/example/needs_input_upload_done.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    write_all(handler, content);
    ASSERT_TRUE(handler.has_output());
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, has_output_upload_done)
{
    std::string upload_path = "www/example/has_output_upload_done.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    write_all(handler, content);
    ASSERT_TRUE(handler.has_output());
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, check_content_uploaded)
{
    std::string upload_path = "www/example/check_content_uploaded.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    write_all(handler, content);
    ASSERT_FALSE(handler.needs_input());
    ASSERT_TRUE(handler.has_output());

    // validating content
    FILE* f = fopen(upload_path.c_str(), "r");
    ASSERT_TRUE(f != NULL);
    char buffer[1028];
    size_t n = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    std::string file_content(buffer, n);
    ASSERT_STREQ(content.c_str(), file_content.c_str());
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, ReturnsUploadSuccess)
{
    std::string upload_path = "www/example/return_201.txt";
    std::string content = "some content to be written to the file";
    UploadHandler handler(upload_path, content.size());
    ASSERT_FALSE(handler.has_output());
    write_all(handler, content);
    ASSERT_TRUE(handler.has_output());
    char buffer[1028];
    size_t n = handler.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    ASSERT_TRUE(http_res.find("201") != std::string::npos);
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, ReturnsFileAlreadyExists)
{
    std::string upload_path = "www/example/return_409.txt";
    std::string content = "some content to be written to the file";
    {
        UploadHandler handler1(upload_path, content.size());
        write_all(handler1, content);
        ASSERT_TRUE(handler1.has_output());
    }
    UploadHandler handler2(upload_path, content.size());
    ASSERT_TRUE(handler2.has_output());
    char buffer[1028];
    size_t n = handler2.read_output(buffer, sizeof(buffer));
    std::string http_res(buffer, n);
    ASSERT_TRUE(http_res.find("409") != std::string::npos);
    std::remove(upload_path.c_str());
}

UTEST(UploadHandlerTest, return_507)
{
    UTEST_SKIP("TODO: Test Disk full without making your machine explode");
}
