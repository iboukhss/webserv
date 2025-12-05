
#include "handler/static_file_handler.hpp"
#include "http/http_request.hpp"
#include "router/router.hpp"
#include "utest/utest.h"

#include <sys/socket.h>

#include <iostream>
#include <string>

UTEST(RouterTest, GET)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, POST)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "POST";
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, DELETE)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, IncorrectMethod)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "INCORRECT METHOD";
    req.path = "/examples/index.html";

    Router router(config);

    Handler* h = router.handle_request(req); // ErrorHandler should be returned
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, EmptyPathReturnsHandler)
{
    // UTEST_SKIP("To do: Map empty request path to root");
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, EmptyPathReturnsDefaultFile)
{
    UTEST_SKIP("To do: Map empty request path to root");
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "";

    Router router(config);

    Handler* h = router.handle_request(req);
    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(h);
    ASSERT_TRUE(sfh != NULL);
    ASSERT_STREQ(sfh->path().c_str(), "www/site1/index.html");
    delete h;
}

UTEST(RouterTest, Root)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/";

    Router router(config);

    Handler* h = router.handle_request(req);
    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(h);
    ASSERT_TRUE(sfh != NULL);
    ASSERT_STREQ(sfh->path().c_str(), "www/site1/index.html");
    delete h;
}

UTEST(RouterTest, BuildRequestPath)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/index.html";

    Router router(config);

    Handler* handler = router.handle_request(req);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler);
    ASSERT_TRUE(sfh != NULL);
    ASSERT_STREQ(sfh->path().c_str(), "www/site1/index.html");
    delete handler;
}

UTEST(RouterTest, BuildRequestPath_Long)
{
    UTEST_SKIP("Implement test on multiple layer folder structure");
    ServerConfig config = make_example_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/example/test_subfolder/test_subfolder/someFile.txt";

    Router router(config);

    Handler* handler = router.handle_request(req);
    ASSERT_TRUE(handler != NULL);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler);
    ASSERT_TRUE(sfh != NULL); // REQUIRED

    ASSERT_STREQ(sfh->path().c_str(), "www/example/test_subfolder/test_subfolder/someFile.txt");

    delete handler;
}
