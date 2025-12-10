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
static ServerConfig make_unittest_config()
{
    RouteConfig loc1, loc2, loc3, loc4;

    loc1.route_path = "/";
    loc1.config.allowed_methods.push_back("GET");

    loc2.route_path = "/files";
    loc2.config.allowed_methods.push_back("DELETE");

    loc3.route_path = "/files/private";
    loc3.config.allowed_methods.push_back("POST");
    loc3.config.uploads_allowed = false;

    loc4.route_path = "/upload";
    loc4.config.allowed_methods.push_back("POST");
    loc4.config.uploads_allowed = true;

    ServerConfig cfg;

    cfg.locations.push_back(loc1);
    cfg.locations.push_back(loc2);
    cfg.locations.push_back(loc3);
    cfg.locations.push_back(loc4);

    return cfg;
}

UTEST(RouterTest, MatchesDefaultRoute)
{
    Router router(make_unittest_config());

    std::string request_path = "/";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/", result.c_str());
}

UTEST(RouterTest, MatchesFilePrefix)
{
    Router router(make_unittest_config());

    std::string request_path = "/files/42.txt";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, NoMatchFallsBackToDefaultRoute)
{
    Router router(make_unittest_config());

    std::string request_path = "/unknown/42.txt";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/", result.c_str());
}

UTEST(RouterTest, MatchesDirectory)
{
    Router router(make_unittest_config());

    std::string request_path = "/files";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, MatchesDirectoryWithTrailingSlash)
{
    Router router(make_unittest_config());

    std::string request_path = "/files/";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, MatchesDirectoryWithMutlipleLeadingSlashes)
{
    UTEST_SKIP("TODO: IMPLEMENT THIS FEATURE");

    Router router(make_unittest_config());

    std::string request_path = "///files";
    std::string result = router.find_best_route(request_path).route_path;

    ASSERT_STREQ("/files", result.c_str());
}

UTEST(RouterTest, ReturnsStaticFileHandler)
{
    Router router(make_unittest_config());

    HttpRequest req;
    req.method = "GET";
    req.path = "/";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<StaticFileHandler*>(h));
    delete h;
}

UTEST(RouterTest, ReturnsUploadHandler)
{
    Router router(make_unittest_config());

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
    Router router(make_unittest_config());

    HttpRequest req;
    req.method = "DELETE";
    req.path = "/files/foo.txt";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<DeleteHandler*>(h));
    delete h;
}

UTEST(RouterTest, UploadsNotAllowed)
{
    Router router(make_unittest_config());

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
    Router router(make_unittest_config());

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
    Router router(make_unittest_config());

    HttpRequest req;
    req.method = "PATCH";
    req.path = "/files/foo.txt";

    Handler* h = router.handle_request(req);
    EXPECT_TRUE(dynamic_cast<ErrorHandler*>(h));
    delete h;
}

UTEST(RouterTest, BuildRequestPath)
{
    UTEST_SKIP("TODO: MOVE THIS TO A BETTER LOCATION");
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/index.html";

    Router router(config);

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

    Router router(config);

    Handler* handler = router.handle_request(req);
    ASSERT_TRUE(handler != NULL);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler);
    ASSERT_TRUE(sfh != NULL); // REQUIRED

    // ASSERT_STREQ("www/example/test_subfolder/test_subfolder/someFile.txt", sfh->path().c_str());

    delete handler;
}
