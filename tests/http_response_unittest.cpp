#include "http/http_response.hpp"
#include "utest/utest.h"

UTEST(HttpResponseTest, BasicString)
{
    UTEST_SKIP("TODO(isma): Need to fix this test!");

    HttpResponse res;

    res.code = HttpResponse::kOk;
    res.headers["Content-Type"] = "text/plain";
    res.body = "Hello, world!\n";
    std::string out = res.to_string();

    ASSERT_STREQ(out.c_str(), "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 14\r\n"
                              "\r\n"
                              "Hello, world!\n");
}
