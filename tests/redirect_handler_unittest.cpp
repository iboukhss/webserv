#include "config/server_config.hpp"
#include "handler/redirect_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

#include <string>

// Helper: read everything the handler outputs
static std::string read_all(Handler& h)
{
    std::string out;
    char buf[2]; // tiny buffer to force multiple reads
    while (h.has_output()) {
        size_t n = h.read_output(buf, sizeof(buf));
        if (n == 0)
            break;
        out.append(buf, n);
    }
    return out;
}

// Helper: make a minimal request object
static HttpRequest make_req(const std::string& method, const std::string& path)
{
    HttpRequest req;
    req.method = method;
    req.path = path;
    req.query_string = "";
    req.content_length = 0;
    return req;
}

UTEST(RedirectHandlerTest, builds_redirect_response_with_location_header)
{
    RouteConfig rc;
    rc.shared.redirect.code = HttpResponse::kStatusMovedPermanently;
    rc.shared.redirect.url = "/new-place";
    HttpRequest req = make_req("GET", "/old");

    RedirectHandler h(rc, req);
    std::string resp = read_all(h);

    // Must contain Location header
    ASSERT_TRUE(resp.find("Location: /new-place") != std::string::npos);

    // Redirect should be 3xx (your current code uses 204, this should fail until fixed)
    // Accept common redirect codes. Pick the one you implement and tighten later if you want.
    bool is_301 = resp.find(" 301 ") != std::string::npos;

    ASSERT_TRUE(is_301);

    // Usually redirects have no body in your implementation
    ASSERT_TRUE(resp.find("Content-Length: 0") == std::string::npos);

    // Must end headers correctly (at least one empty line)
    ASSERT_TRUE(resp.find("\r\n\r\n") != std::string::npos ||
                resp.find("\n\n") != std::string::npos);
}

UTEST(RedirectHandlerTest, streams_output_in_multiple_reads)
{
    RouteConfig rc;
    rc.shared.redirect.code = HttpResponse::kStatusMovedPermanently;
    rc.shared.redirect.url = "/somewhere";
    HttpRequest req = make_req("GET", "/old");
    RedirectHandler h(rc, req);

    // Read with very small chunks to ensure handler offsets are correct
    std::string resp1;
    char buf[3];
    while (h.has_output()) {
        size_t n = h.read_output(buf, sizeof(buf));
        if (n == 0)
            break;
        resp1.append(buf, n);
    }

    // Should be done now
    ASSERT_TRUE(h.is_done());
    ASSERT_FALSE(h.has_output());

    // Subsequent reads should be 0
    ASSERT_EQ((size_t) 0, h.read_output(buf, sizeof(buf)));

    // Basic sanity: contains Location
    ASSERT_TRUE(resp1.find("Location: /somewhere") != std::string::npos);
}

UTEST(RedirectHandlerTest, write_input_is_noop)
{
    RouteConfig rc;
    rc.shared.redirect.code = HttpResponse::kStatusMovedPermanently;
    rc.shared.redirect.url = "/newPath";
    HttpRequest req = make_req("GET", "/old");

    RedirectHandler h(rc, req);

    const char* junk = "abc";
    ASSERT_EQ((size_t) 0, h.write_input(junk, 3));

    // Still has output (write_input shouldn't affect it)
    ASSERT_TRUE(h.has_output());
}
