
#include "config/server_config.hpp"
#include "handler/error_handler.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"

#include <map>

UTEST(ErrorHandlerTest, is_done)
{
    RouteConfig rc;
    ErrorHandler handler(HttpResponse::kStatusBadRequest, rc);
    ASSERT_FALSE(handler.is_done());
}

UTEST(ErrorHandlerTest, has_output)
{
    RouteConfig rc;
    ErrorHandler handler(HttpResponse::kStatusNotFound, rc);
    ASSERT_TRUE(handler.has_output());
}

UTEST(ErrorHandlerTest, needs_input)
{
    RouteConfig rc;
    ErrorHandler handler(HttpResponse::kStatusInternalServerError, rc);
    ASSERT_FALSE(handler.needs_input());
}

UTEST(ErrorHandlerTest, error_code)
{
    RouteConfig rc;
    ErrorHandler handler(HttpResponse::kStatusInternalServerError, rc);
    ASSERT_EQ(handler.error_code(), 500);
}

UTEST(ErrorHandlerTest, make_error_html)
{
    RouteConfig rc;
    ErrorHandler handler(HttpResponse::kStatusInternalServerError, rc);

    char buf[1028];
    size_t n;
    std::string error_html;
    while (handler.has_output()) {
        n = handler.read_output(buf, sizeof(buf));
        error_html.append(buf, n);
    }
    ASSERT_EQ(handler.error_code(), 500);
    std::string expected =
        HttpResponse::make_error(HttpResponse::kStatusInternalServerError, rc.shared.error_pages)
            .to_string();
    ASSERT_STREQ(error_html.data(), expected.data());
}

UTEST(ErrorHandlerTest, error_html_uses_custom_error_page_mapping)
{
    RouteConfig rc;

    rc.shared.error_pages[HttpResponse::kStatusInternalServerError] =
        "<html><body>CUSTOM_500_MARKER</body></html>";

    ErrorHandler handler(HttpResponse::kStatusInternalServerError, rc);

    char buf[1028];
    std::string out;
    while (handler.has_output()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        out.append(buf, n);
    }

    ASSERT_TRUE(out.find("CUSTOM_500_MARKER") != std::string::npos);

    std::string expected =
        HttpResponse::make_error(HttpResponse::kStatusInternalServerError, rc.shared.error_pages)
            .to_string();
    ASSERT_STREQ(out.c_str(), expected.c_str());
}
