#ifndef HTTP_HTTP_REQUEST_HPP_
#define HTTP_HTTP_REQUEST_HPP_

#include "core/server_defaults.hpp"
#include "http/http_version.hpp"

#include <map>
#include <string>

struct HttpRequest {
    explicit HttpRequest(HttpVersion protocol = WEBSERV_DEFAULT_HTTP_VERSION);

    std::string method;
    std::string path;
    std::string query_string;
    HttpVersion http_version;

    // Try to keep feature parity with HttpResponse if applicable here.
    size_t content_length;
    bool is_chunked;
    bool keep_alive;

    // Useless headers we do not support, but might need eventually.
    std::map<std::string, std::string> headers;

    std::string body;
};

#endif // HTTP_HTTP_REQUEST_HPP_
