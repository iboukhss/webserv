#ifndef HTTP_HTTP_PARSER_HPP_
#define HTTP_HTTP_PARSER_HPP_

#include "http/http_request.hpp"

#include <string>

class HttpParser {
public:
    HttpParser();

    enum Status { kError = 0, kIncomplete, kRequestLineDone, kHeadersDone, kBodyDone };

    void feed_data(const char* buf, size_t n);
    size_t slurp_data(char* buf, size_t n);

    Status status() { return status_; }
    const HttpRequest& request() { return req_; }

private:
    void parse_request_line();
    void parse_headers();
    void parse_body();

    Status status_;
    std::string buffer_;
    size_t bytes_slurped_;
    HttpRequest req_;
};

#endif // HTTP_HTTP_PARSER_HPP_
