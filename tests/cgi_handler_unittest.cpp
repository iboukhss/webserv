#include "config/server_config.hpp"
#include "handler/cgi_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class TempCgiScript {
public:
    explicit TempCgiScript(const std::string& content)
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
    TempCgiScript(const TempCgiScript&) {}
    TempCgiScript& operator=(const TempCgiScript&) { return *this; }
    static std::string to_string(int v)
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
};

void print_response_expected(const std::string& response, const std::string& expected)
{
    std::cout << "reponse = " << std::endl << response << std::endl;
    std::cout << "expected = " << std::endl << expected << std::endl;
    std::cout << "response size = " << response.size() << std::endl;
    std::cout << "expected size = " << expected.size() << std::endl;
}

UTEST(CgiHandler, SimpleGet)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo \"Content-Length: 9\"\n"
                         "echo\n"
                         "echo -n \"Hello CGI\"");
    RouteConfig rc;
    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    CgiHandler handler(script.path(), rc, req);

    char buf[1024];
    std::string response;
    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    res.inline_body = "Hello CGI";

    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PostBody)
{
    const char* body = "hello world"; // update Content-Length in script if body changes
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo \"Content-Length: 11\"\n"
                         "echo\n"
                         "cat -");
    RouteConfig rc;
    HttpRequest req;
    req.method = "POST";
    req.path = script.path();
    req.query_string = "";
    req.content_length = strlen(body); // length of body

    CgiHandler handler(script.path(), rc, req);

    while (handler.needs_input()) {
        handler.write_input(body, req.content_length);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    res.inline_body = body;

    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, MissingContentType)
{
    TempCgiScript script("echo\n"
                         "echo \"No headers\"");
    RouteConfig rc;
    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    CgiHandler handler(script.path(), rc, req);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res = res.make_error(HttpResponse::kStatusBadGateway, rc.shared.error_pages, req);
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

class TempFile {
public:
    explicit TempFile(const std::string& path)
        : path_(path)
    {
    }
    ~TempFile() { ::remove(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    TempFile(const TempFile&) {}
    TempFile& operator=(const TempFile&) { return *this; }
    std::string path_;
};

UTEST(CgiHandler, NotExecutable)
{
    TempFile f("/tmp/not_exec.py");
    {
        std::ofstream ofs(f.path().c_str());
        ofs << "echo test\n";
    }
    chmod(f.path().c_str(), 0644);
    RouteConfig rc;
    HttpRequest req;
    req.method = "GET";
    req.path = f.path();
    req.query_string = "";
    req.content_length = 0;

    CgiHandler handler(f.path(), rc, req);

    char buf[512];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res = res.make_error(HttpResponse::kStatusForbidden, rc.shared.error_pages, req);
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, EmptyOutput)
{
    TempCgiScript script("echo \"Content-Type: text/html; charset=UTF-8\"\n"
                         "echo\n");
    RouteConfig rc;
    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;


    CgiHandler handler(script.path(), rc, req);

    char buf[128];
    size_t n = 0;
    std::string response;
    while (!handler.is_done()) {
        n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res = HttpResponse::make_response_headers_only(HttpResponse::kStatusOk, "", 0, req);
    std::string expected = res.to_string();
    print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_GET)
{
    std::string script = "tests/cgi_upper.py";
    std::string query = "hello=world=returned=UPPER=case";
    RouteConfig rc;
    HttpRequest req;
    req.method = "GET";
    req.path = script;
    req.query_string = query;
    req.content_length = 0;



    CgiHandler handler(script, rc, req);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = query;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_POST)
{
    std::string script = "tests/cgi_upper.py";
    const char* body = "hello=world=returned=UPPER=case=after=POST";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "SOME=QUERY=STRING";
    req.content_length = body_len;

    RouteConfig rc;

    CgiHandler handler(script, rc, req);

    // Write POST body
    while (handler.needs_input()) {
        handler.write_input(body, body_len);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = body;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_POST_1MB_Payload)
{

    // UTEST_SKIP("TODO: output pipe closed to early, fix required");
    std::string script = "tests/cgi_upper.py";
    const size_t body_len = 1024 * 1024; // 1 MB
    std::string body(body_len, 'a');

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "SOME=QUERY=STRING";
    req.content_length = body_len;

    RouteConfig rc;

    CgiHandler handler(script, rc, req);

    // Write POST body (will require multiple writes)
    size_t written = 0;
    while (handler.needs_input()) {
        size_t chunk = body_len - written;
        size_t n = handler.write_input(body.data() + written, chunk);
        written += n;
    }

    ASSERT_EQ(written, body_len);

    char buf[4096];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = body;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(Config, Bla_Rejects_Get)
{
    HttpConfig http = load_http_config("config/youpi_banane.conf");

    ASSERT_EQ(http.servers.size(), 1u);

    const ServerConfig& cfg = http.servers[0];

    ASSERT_TRUE(std::find(cfg.shared.cgi.allowed_methods.begin(),
                          cfg.shared.cgi.allowed_methods.end(),
                          "GET") == cfg.shared.cgi.allowed_methods.end());
}

UTEST(Config, Bla_Cgi_Exec_Path)
{
    HttpConfig http = load_http_config("config/youpi_banane.conf");
    const ServerConfig& cfg = http.servers[0];
    ASSERT_TRUE(cfg.shared.cgi.exec_path == "/tests/ubuntu_cgi_tester");
}

UTEST(Config, PostBody_MaxBody)
{
    HttpConfig http = load_http_config("config/youpi_banane.conf");
    const ServerConfig& cfg = http.servers[0];

    std::map<std::string, RouteConfig>::const_iterator it = cfg.locations.find("/post_body");
    ASSERT_TRUE(it != cfg.locations.end());
    ASSERT_EQ(it->second.shared.max_body_size, 100u);
}

UTEST(Config, Directory_Index)
{
    HttpConfig http = load_http_config("config/youpi_banane.conf");
    const ServerConfig& cfg = http.servers[0];

    std::map<std::string, RouteConfig>::const_iterator it = cfg.locations.find("/directory/");
    ASSERT_TRUE(it != cfg.locations.end());

    ASSERT_EQ(it->second.shared.index_files.size(), 1u);
    ASSERT_TRUE(it->second.shared.index_files[0] == "youpi.bad_extension");
}

UTEST(Config, Directory_Inherits_Get_Only)
{
    HttpConfig http = load_http_config("config/youpi_banane.conf");
    const ServerConfig& cfg = http.servers[0];

    std::map<std::string, RouteConfig>::const_iterator it = cfg.locations.find("/directory/");
    ASSERT_TRUE(it != cfg.locations.end());

    const RouteConfig& dir = it->second;
    ASSERT_TRUE(std::find(dir.shared.allowed_methods.begin(),
                          dir.shared.allowed_methods.end(),
                          "GET") != dir.shared.allowed_methods.end());
}

UTEST(CgiHandler, UbuntuCgiTester_GET)
{
    // UTEST_SKIP("TODO: to implement");
    HttpRequest req;
    req.method = "GET";
    req.path = "/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = 0;

    RouteConfig rc;
    rc.shared.cgi.exec_path = "";

    CgiHandler handler("tests/ubuntu_cgi_tester", rc, req);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    // std::cout << response;
    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_POST)
{
    // UTEST_SKIP("TODO: to implement");
    const char* body = "hello";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = "/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = body_len;

    RouteConfig rc;

    CgiHandler handler("tests/ubuntu_cgi_tester", rc, req);

    while (handler.needs_input())
        handler.write_input(body, body_len);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    // std::cout << response;
    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_ExtensionBased)
{
    TempCgiScript dummy("echo should not run");
    std::string fake_file = dummy.path() + ".bla";
    ::rename(dummy.path().c_str(), fake_file.c_str());

    HttpRequest req;
    req.method = "POST";
    req.path = fake_file;
    req.query_string = "";
    req.content_length = 4;

    RouteConfig rc;
    rc.shared.cgi.extension = ".bla";
    rc.shared.cgi.exec_path = "/tests/ubuntu_cgi_tester";
    rc.shared.cgi.allowed_methods.push_back("POST");

    CgiHandler handler(fake_file, rc, req);

    handler.write_input("test", 4);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_MethodNotAllowed)
{
    HttpRequest req;
    req.method = "GET"; // forbidden
    req.path = "/tests/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = 0;

    RouteConfig rc;
    rc.shared.cgi.allowed_methods.push_back("POST");

    CgiHandler handler(req.path, rc, req);

    char buf[512];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("405") != std::string::npos ||
                response.find("403") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_NotExecutable)
{
    TempFile f("/tmp/not_exec_cgi");
    {
        std::ofstream ofs(f.path().c_str());
        ofs << "binary";
    }
    chmod(f.path().c_str(), 0644);

    HttpRequest req;
    req.method = "GET";
    req.path = f.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig rc;

    CgiHandler handler(f.path(), rc, req);

    char buf[256];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_output(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("403") != std::string::npos);
}
