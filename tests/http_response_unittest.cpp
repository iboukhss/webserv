#include "http/http_response.hpp"
#include "utest/utest.h"

UTEST(HttpResponseTest, BasicString)
{
    HttpResponse res;

    res.status = 200;
    res.content_type = "text/plain";
    res.headers["Content-Type"] = "text/plain";
    res.body = "Hello World!";
    std::string out = res.to_string();

    ASSERT_STREQ(out.c_str(), "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 14\r\n"
                              "\r\n"
                              "Hello, world!\n");
}
