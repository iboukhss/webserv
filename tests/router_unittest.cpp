#include "config/server_config.hpp"
#include "handler/delete_handler.hpp"
#include "handler/error_handler.hpp"
#include "handler/static_file_handler.hpp"
#include "handler/upload_handler.hpp"
#include "http/http_request.hpp"
#include "router/router.hpp"
#include "utest/utest.h"

#include <sys/socket.h>

#include <string>

// This is very brittle and horrible
static HttpConfig make_unittest_config()
{
    return load_http_config("tests/config/router_unittest.conf");
}

UTEST(RouterTest, MatchesDefaultRoute)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    std::string request_path = "/";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/", result.c_str());
}

UTEST(RouterTest, MatchesFilePrefix)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    std::string request_path = "/files/42.txt";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, NoMatchFallsBackToDefaultRoute)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    std::string request_path = "/unknown/42.txt";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/", result.c_str());
}

UTEST(RouterTest, MatchesDirectory)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    std::string request_path = "/files";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, MatchesDirectoryWithTrailingSlash)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);
    std::string request_path = "/files/";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, MatchesDirectoryWithMutlipleLeadingSlashes)
{
    UTEST_SKIP("TODO: IMPLEMENT THIS FEATURE");

    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    std::string request_path = "///files";
    std::string result = router.find_best_route(request_path).path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, ReturnsStaticFileHandler)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "GET";
    req.path = "/";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<StaticFileHandler*>(h));
    delete h;
}

UTEST(RouterTest, ReturnsUploadHandler)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "POST";
    req.path = "/upload";
    req.query_string = "file=42.txt";
    req.content_length = 42;

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<UploadHandler*>(h));
    delete h;
}

UTEST(RouterTest, ReturnsDeleteHandler)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "DELETE";
    req.path = "/files/foo.txt";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<DeleteHandler*>(h));
    delete h;
}

UTEST(RouterTest, UploadsNotAllowed)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "POST";
    req.path = "/files/private";
    req.query_string = "file=42.txt";
    req.content_length = 42;

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<ErrorHandler*>(h));
    delete h;
}

UTEST(RouterTest, EmptyPostBypassesUploadRestriction)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "POST";
    req.path = "/files/private";
    req.content_length = 0;

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<UploadHandler*>(h));
    delete h;
}
UTEST(RouterTest, UnsupportedMethodReturnsErrorHandler)
{
    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    HttpRequest req;
    req.method = "PATCH";
    req.path = "/files/foo.txt";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<ErrorHandler*>(h));
    delete h;
}

/*
UTEST(RouterTest, BuildRequestPath)
{
    UTEST_SKIP("TODO: MOVE THIS TO A BETTER LOCATION");
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/index.html";

    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    Handler* handler = router.handle_request(req);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler);
    ASSERT_TRUE(sfh != NULL);
    // ASSERT_STREQ("www/site1/index.html", sfh->path().c_str());
    delete handler;
}

UTEST(RouterTest, BuildRequestPath_Long)
{
    UTEST_SKIP("TODO: MOVE THIS TO A BETTER LOCATION");
    ServerConfig config = make_example_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/example/test_subfolder/test_subfolder/someFile.txt";

    HttpConfig conf = make_unittest_config();
    Router router(conf.servers[0].locations);

    Handler* handler = router.handle_request(req);
    ASSERT_TRUE(handler != NULL);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler);
    ASSERT_TRUE(sfh != NULL); // REQUIRED

    // ASSERT_STREQ("www/example/test_subfolder/test_subfolder/someFile.txt", sfh->path().c_str());

    delete handler;
}
*/
