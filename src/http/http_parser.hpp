#ifndef HTTP_HTTP_PARSER_HPP_
#define HTTP_HTTP_PARSER_HPP_

#include "http/http_request.hpp"

#include <string>

#define WEBSERV_MAX_RECVBUF_SIZE (16 * 1024)
#define WEBSERV_MAX_SENDBUF_SIZE (16 * 1024)
#define WEBSERV_MAX_CHUNK_SIZE   (10 * 1024)

class HttpParser {
public:
    enum State {
        kParsingError = 0,
        kParsingRequestLine,
        kParsingHeaders,
        kParsingChunkSize,
        kParsingBody,
        kParsingChunkCrlf,
        kParsingDone
    };

    HttpParser();

    void append_data(const char* src, size_t n);
    size_t read_next_body_chunk(char* dest, size_t n) const;
    void consume_body_chunk(size_t n);
    void reset_for_next_request();

    size_t available_size() const { return WEBSERV_MAX_RECVBUF_SIZE - recvbuf_.size(); }
    bool did_parse_request_line() const { return state_ > HttpParser::kParsingRequestLine; }
    bool did_parse_headers() const { return state_ > HttpParser::kParsingHeaders; }
    bool is_done() const { return state_ == HttpParser::kParsingDone; }
    bool has_body_chunk() const;
    bool has_error() const { return state_ == HttpParser::kParsingError; }
    const HttpRequest& request() const { return req_; }

private:
    size_t available_chunk_size() const;

    void parse_request_line();
    void parse_headers();
    void parse_chunk_size();
    void parse_chunk_crlf();

    HttpParser::State state_;
    HttpRequest req_;
    std::string recvbuf_;
    size_t current_chunk_size_;
    size_t current_chunk_consumed_;
};

#endif // HTTP_HTTP_PARSER_HPP_
