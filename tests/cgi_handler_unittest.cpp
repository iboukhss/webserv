#include "config/server_config.hpp"
#include "handler/cgi_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class TempCgiScript {
public:
    TempCgiScript(const std::string& content)
    {
        static int idx = 0;
        path_ = "/tmp/cgi_test_" + to_string(idx++) + ".sh";

        std::ofstream ofs(path_.c_str());
        ofs << "#!/bin/sh\n";
        ofs << content;
        ofs.close();

        chmod(path_.c_str(), 0755);
    }

    ~TempCgiScript() { ::remove(path_.c_str()); }

    const std::string& path() const { return path_; }

private:
    std::string path_;

    static std::string to_string(int v)
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
};

UTEST(CgiHandler, SimpleGet)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n"
                         "echo \"Hello CGI\"");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    char buf[1024];
    std::string response;
    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    ASSERT_TRUE(response.find("Hello CGI") != std::string::npos);
}

UTEST(CgiHandler, PostBody)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n"
                         "cat -");

    HttpRequest req;
    req.method = "POST";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 11;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    const char* body = "hello world";

    while (handler.needs_input()) {
        handler.write_data(body, 11);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("hello world") != std::string::npos);
}

UTEST(CgiHandler, MissingContentType)
{
    TempCgiScript script("echo\n"
                         "echo \"No headers\"");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("502") != std::string::npos);
}

class TempFile {
public:
    TempFile(const std::string& path)
        : path_(path)
    {
    }
    ~TempFile() { ::remove(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

UTEST(CgiHandler, NotExecutable)
{
    TempFile f("/tmp/not_exec.py");
    {
        std::ofstream ofs(f.path().c_str());
        ofs << "echo test\n";
    }
    chmod(f.path().data(), 0644);

    HttpRequest req;
    req.method = "GET";
    req.path = f.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    CgiHandler handler(f.path(), req, cfg);

    char buf[512];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("403") != std::string::npos);
}

UTEST(CgiHandler, EmptyOutput)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    CgiHandler handler(script.path(), req, cfg);

    char buf[128];
    size_t total = 0;

    while (!handler.is_done()) {
        total += handler.read_data(buf, sizeof(buf));
    }

    ASSERT_TRUE(total >= 0); // should terminate cleanly
}

UTEST(CgiHandler, RealScript_GET)
{
    std::string script = "tests/cgi_upper.py";

    HttpRequest req;
    req.method = "GET";
    req.path = script;
    req.query_string = "hello=world";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script, req, cfg);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    // Body should contain uppercased query string
    ASSERT_TRUE(response.find("HELLO=WORLD") != std::string::npos);

    // Optional sanity checks
    ASSERT_TRUE(response.find("200") != std::string::npos);
}

UTEST(CgiHandler, RealScript_POST)
{
    std::string script = "tests/cgi_upper.py";

    const char* body = "Hello CGI";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "";
    req.content_length = body_len;

    RouteConfig cfg;

    CgiHandler handler(script, req, cfg);

    // Write POST body
    while (handler.needs_input()) {
        handler.write_data(body, body_len);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    // std::cout << response << std::endl;
    ASSERT_TRUE(response.find("HELLO CGI") != std::string::npos);
}
