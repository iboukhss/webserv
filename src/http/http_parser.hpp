#ifndef HTTP_HTTP_PARSER_HPP_
#define HTTP_HTTP_PARSER_HPP_

#include "http/http_request.hpp"

#include <string>

class HttpParser {
public:
    enum State {
        kParsingError = 0,
        kParsingRequestLine,
        kParsingHeaders,
        kParsingBody,
        kParsingDone
    };

    HttpParser();

    void append_data(const char* src, size_t n);
    size_t read_body_chunk(char* dest, size_t n);
    void reset_for_next_request();

    HttpParser::State state() { return state_; }
    const HttpRequest& request() { return req_; }

private:
    void parse_request_line();
    void parse_headers();
    void parse_body();

    HttpParser::State state_;
    std::string raw_buffer_;
    std::string body_buffer_;
    size_t body_bytes_parsed_;
    size_t body_bytes_read_;
    HttpRequest req_;
};

#endif // HTTP_HTTP_PARSER_HPP_
