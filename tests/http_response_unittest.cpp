#include "http/http_response.hpp"
#include "utest/utest.h"

UTEST(HttpResponseTest, BasicStringHttp1_0)
{
    HttpResponse res(kHttpVersion1_0);
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.inline_body = "Hello, world!\n";

    std::string out = res.to_string();

    ASSERT_STREQ(out.c_str(), "HTTP/1.0 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 14\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Hello, world!\n");
}
